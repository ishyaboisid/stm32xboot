import serial
import struct
import sys
from pathlib import Path
from Crypto.Cipher import AES
import encrypt_firmware
import sign_firmware
import keygen
import time

PORT       = "/dev/tty.usbmodem103"
BAUDRATE   = 115200
CHUNK_SIZE = 1024

BASE = Path("/Users/siddharthmanikant/Desktop/Bachelor_Project/Application/build")

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

def send_firmware(ser):
        # send 4-byte header, and fw ver, MCU decides slot
        print("Sending header (0xAA 0xBB) and fw ver...")
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
        expect_ack(ser, "header")

        # now we know the slot — load the correct binary
        bin_path = SLOT_PATHS[slot]
        plaintext = open(bin_path, "rb").read()

        # step 1: sign the plaintext — appends 64-byte signature to end
        signed_path   = '/Users/siddharthmanikant/Desktop/Bachelor_Project/Tools/Application.signed.bin'
        encrypted_path = '/Users/siddharthmanikant/Desktop/Bachelor_Project/Tools/Application.enc.bin'
        private_key_path = '/Users/siddharthmanikant/Desktop/Bachelor_Project/Tools/private_key.pem'

        sign_firmware.sign_firmware(private_key_path, bin_path, signed_path)

        # step 2: encrypt the signed binary
        iv = encrypt_firmware.encrypt_firmware(signed_path, encrypted_path)

        # step 3: CRC over signed plaintext (what MCU has after decryption)
        signed_plaintext = open(signed_path, "rb").read()
        crc = compute_crc32_stm32(signed_plaintext)

        # step 4: send the encrypted binary
        data = open(encrypted_path, "rb").read()
        
        print(f"Slot {slot} assigned")
        print(f"Firmware : {len(data)} bytes")
        print(f"CRC32    : 0x{crc:08X}")

        # size
        print(f"Sending SIZE ({len(data)} bytes)...")
        ser.write(struct.pack("<I", len(data)))
        expect_ack(ser, "size")

        ser.write(iv)
        expect_ack(ser, "IV")

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
        expect_ack(ser, "whole data")

        # CRC
        print(f"Sending CRC (0x{crc:08X})...")
        ser.write(struct.pack("<I", crc))
        expect_ack(ser, "CRC")

        expect_ack(ser, "signature") # new

        print("Waiting for final result...")
        result = wait_response(ser)
        if result == "OK":
            print(f"\nSuccess — slot {slot} firmware written to flash, MCU rebooting")
            print(data[:64].hex()) 
            print(plaintext[:64].hex()) 
        else:
            print(f"\nFailed — MCU replied: {result}")
            sys.exit(1)

if __name__ == "__main__":
    keygen.keygen # run again before reconnecting power to mcu after power loss 
    while True:
        try:
            ser = serial.Serial(PORT, BAUDRATE, timeout=0.2)
            print(f"Connected to {PORT}")
            break
        except serial.SerialException:
            print("Waiting for device...")
            time.sleep(1)
    with serial.Serial(PORT, BAUDRATE, timeout=0.2) as ser:
        waiting_for_ready(ser, timeout_s=10)
        send_firmware(ser)