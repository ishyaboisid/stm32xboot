import serial
import struct
import sys
from pathlib import Path
from Crypto.Cipher import AES
import encrypt_firmware
import sign_firmware
import keygen

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

AES_IV = bytes([
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
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

def expect_ack(ser, context: str):
    response = wait_response(ser)
    if response != "ACK":
        print(f"  NACK or unexpected response at: {context}")
        sys.exit(1)

def send_firmware(port: str):
    with serial.Serial(port, BAUDRATE, timeout=15) as ser:

        print("\nWaiting for READY...")
        while True:
            line = ser.readline().decode(errors="replace").strip()
            print(f"  MCU: {line}")
            if line == "READY":
                break

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
        encrypt_firmware.encrypt_firmware(signed_path, encrypted_path)

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
    keygen.keygen
    send_firmware(PORT)