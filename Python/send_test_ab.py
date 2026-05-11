import serial
import struct
import sys
from pathlib import Path

PORT       = "/dev/tty.usbmodem103"
BAUDRATE   = 115200
CHUNK_SIZE = 1024

BASE = Path("/Users/siddharthmanikant/Desktop/Bachelor_Project/Application/build")

SLOT_PATHS = {
    "A": BASE / "slotA" / "Application.bin",
    "B": BASE / "slotB" / "Application.bin",
}

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

        # send 2-byte header — MCU decides slot
        print("Sending header (0xAA 0xBB)...")
        ser.write(bytes([0xAA, 0xBB]))

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
        print(f"Slot {slot} assigned — loading {bin_path}")
        data = open(bin_path, "rb").read()
        crc  = compute_crc32_stm32(data)
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

        print("Waiting for final result...")
        result = wait_response(ser)
        if result == "OK":
            print(f"\nSuccess — slot {slot} firmware written to flash, MCU rebooting")
        else:
            print(f"\nFailed — MCU replied: {result}")
            sys.exit(1)

if __name__ == "__main__":
    send_firmware(PORT)