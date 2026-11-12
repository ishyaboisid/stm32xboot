import serial
import struct
import sys
from pathlib import Path
from Crypto.Cipher import AES
import encrypt_firmware
import sign_firmware
import keygen
import time
import argparse
from timing_report import UpdateTimer

# PORT       = "/dev/tty.usbmodem103"
# BAUDRATE   = 115200
CHUNK_SIZE = 1024

BASE = Path("../app/build")

SLOT_PATHS = {
    "A": BASE / "slotA" / "Application.bin",
    "B": BASE / "slotB" / "Application.bin",
}

FW_VERSION = (1,3)

AES_KEY = bytes([
    0x2B, 0x7E, 0x15, 0x16,
    0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88,
    0x09, 0xCF, 0x4F, 0x3C
])

def parse_args():
    parser = argparse.ArgumentParser(
        description="Firmware uploader"
    )

    parser.add_argument(
        "--port",
        required=True,
        help="Serial port (e.g. /dev/tty.usbmodem103)"
    )

    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Baud rate (default: 115200)"
    )

    parser.add_argument(
        "--csv",
        default=None,
        help="Append successful update timing results to this CSV file"
    )

    return parser.parse_args()

def compute_crc32_stm32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    remainder = len(data) % 4
    if remainder:
        data = data + b'\xFF' * (4 - remainder)
    for i in range(0, len(data), 4):
        word = struct.unpack(">I", data[i:i+4])[0]
        crc ^= word
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc

def wait_response(ser) -> str:
    line = ser.readline().decode(errors="replace").strip()
    print(f"  MCU: {line}")
    return line

def waiting_for_ready(ser, timeout_s=5):
    print("\nWaiting for READY...")
    start = time.monotonic()

    while True:
        if time.monotonic() - start > timeout_s:
            raise TimeoutError("READY timeout")

        line = ser.readline().decode(errors="replace").strip()
        if not line:
            continue

        print(f"  MCU: {line}")

        if line == "READY":
            return

def expect_ack(ser, context, timeout_s=5000):
    start = time.monotonic()

    while True:
        if time.monotonic() - start > timeout_s:
            raise TimeoutError(f"ACK timeout at {context}")

        line = ser.readline().decode(errors="replace").strip()
        if not line:
            continue

        print(f"  MCU: {line}")

        if line == "ACK":
            return

        raise RuntimeError(f"Unexpected response at {context}: {line}")

def send_firmware(ser, baudrate=115200, csv_path=None):
        tm = UpdateTimer()

        # send 4-byte header, and fw ver, MCU decides slot
        print("Sending header (0xAA 0xBB) and fw ver...")
        tm.mark("start")
        ser.write(bytes([0xAA, 0xBB, FW_VERSION[0], FW_VERSION[1]]))

        # MCU responds with A or B — load the correct binary
        print("Waiting for slot assignment...")
        slot = None
        while True:
            line = ser.readline().decode(errors="replace").strip()
            print(f"  MCU: {line}")
            if line in ("A", "B"):
                slot = line
                break
        tm.mark("slot")
        expect_ack(ser, "header")

        # now we know the slot — load the correct binary
        tm.mark("prep0")
        bin_path = SLOT_PATHS[slot]
        plaintext = open(bin_path, "rb").read()

        # step 1: sign the plaintext — appends 64-byte signature to end
        signed_path   = 'Application.signed.bin'
        encrypted_path = 'Application.enc.bin'
        private_key_path = 'private_key.pem'

        sign_firmware.sign_firmware(private_key_path, bin_path, signed_path)
        tm.mark("signed")

        # step 2: encrypt the signed binary
        iv = encrypt_firmware.encrypt_firmware(signed_path, encrypted_path)
        tm.mark("encrypted")

        # step 3: CRC over signed plaintext (what MCU has after decryption)
        signed_plaintext = open(signed_path, "rb").read()
        crc = compute_crc32_stm32(signed_plaintext)
        tm.mark("crc")

        # step 4: send the encrypted binary
        data = open(encrypted_path, "rb").read()
        tm.mark("prep1")

        print(f"Slot {slot} assigned")
        print(f"Firmware : {len(data)} bytes")
        print(f"CRC32    : 0x{crc:08X}")

        # size
        print(f"Sending SIZE ({len(data)} bytes)...")
        tm.mark("xfer0")
        ser.write(struct.pack("<I", len(data)))
        expect_ack(ser, "size")

        print(f"Sending IV...")
        ser.write(iv)
        expect_ack(ser, "IV")
        tm.mark("iv_ack")

        # data chunks
        print("Sending DATA...")
        total     = len(data)
        offset    = 0
        chunk_num = 0
        while offset < total:
            chunk = data[offset:offset + CHUNK_SIZE]
            ser.write(chunk)
            expect_ack(ser, f"chunk {chunk_num}")
            offset    += len(chunk)
            chunk_num += 1
            print(f"  Progress: {offset}/{total} bytes", end="\r")
        print()
        tm.mark("data1")
        expect_ack(ser, "whole data")
        tm.mark("done_ack")

        # CRC
        print(f"Sending CRC (0x{crc:08X})...")
        tm.mark("crc0")
        ser.write(struct.pack("<I", crc))
        expect_ack(ser, "CRC")
        tm.mark("crc_ack")

        expect_ack(ser, "signature") # new
        tm.mark("sig_ack")

        print("Waiting for final result...")
        result = wait_response(ser)
        tm.mark("end")
        if result == "OK":
            print(f"\nSuccess — slot {slot} firmware written to flash, MCU rebooting")
            print(data[:64].hex())
            print(plaintext[:64].hex())
            tm.report("firmware", slot, len(data), baudrate, csv_path)
        else:
            print(f"\nFailed — MCU replied: {result}")
            sys.exit(1)

if __name__ == "__main__":
    args = parse_args()

    PORT = args.port
    BAUDRATE = args.baudrate

    while True:
        try:
            # wait until the device is physically connected
            ser = None
            while ser is None:
                try:
                    ser = serial.Serial(PORT, BAUDRATE, timeout=0.2)
                    print(f"Connected to {PORT}")
                except serial.SerialException:
                    print("Waiting for device...")
                    time.sleep(1)

            with ser:
                waiting_for_ready(ser, timeout_s=10)
                send_firmware(ser, BAUDRATE, args.csv)

            # success — exit the outer loop
            break

        except (serial.SerialException, OSError) as e:
            # power loss / USB disconnect / port disappeared mid-transfer
            print(f"\nConnection lost: {e}")
            print("Reconnecting...\n")
            time.sleep(1)
            continue

        except TimeoutError as e:
            # READY or ACK timeout — board may have reset or lost power
            print(f"\nTimeout: {e}")
            print("Retrying from the beginning...\n")
            time.sleep(1)
            continue

        except RuntimeError as e:
            # unexpected response (e.g. NACK) — don't loop forever silently,
            # but still retry since power loss can cause garbage on the line
            print(f"\nProtocol error: {e}")
            print("Retrying from the beginning...\n")
            time.sleep(1)
            continue
