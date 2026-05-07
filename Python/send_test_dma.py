# @file send_test_dma.py

import serial
import struct
import sys
import time
PORT       = "/dev/tty.usbmodem103"
BAUDRATE   = 115200
CHUNK_SIZE = 1024

# def compute_crc32_stm32(data: bytes) -> int:
#     crc = 0xFFFFFFFF
#     remainder = len(data) % 4
#     if remainder:
#         data = data + b'\xFF' * (4 - remainder)
#     for i in range(0, len(data), 4):
#         word = struct.unpack(">I", data[i:i+4])[0]
#         crc ^= word
#         for _ in range(32):
#             if crc & 0x80000000:
#                 crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
#             else:
#                 crc = (crc << 1) & 0xFFFFFFFF
#     return crc

def compute_crc32_stm32(data: bytes) -> int: # https://github.com/Steppeschool/stm32-custom-bootloader/tree/main
    crc = 0xFFFFFFFF

    for byte in data:
        crc ^= byte << 24

        for _ in range(8):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF

    return crc #& 0xFFFFFFFF

def wait_response(ser) -> str:
    line = ser.readline().decode(errors="replace").strip()
    print(f"  MCU: {line}")
    return line

def expect_ack(ser, context: str):
    response = wait_response(ser)
    if response != "ACK":
        print(f"  NACK or unexpected response at: {context}")
        sys.exit(1)

def send_firmware(port: str, data: bytes):
    crc = compute_crc32_stm32(data)

    print(f"Firmware : {len(data)} bytes")
    print(f"CRC32    : 0x{crc:08X}")

    with serial.Serial(port, BAUDRATE, timeout=100000) as ser:

        # wait for READY — consume and print all debug lines until we see it
        print("\nWaiting for READY...")
        while True:
            line = ser.readline().decode(errors="replace").strip()
            print(f"  MCU: {line}")
            if line == "READY":
                break

        # send full 6-byte header atomically — MCU receives this in one DMA transfer
        # [0xAA] [0xBB] [SIZE: 4 bytes little-endian]
        print(f"Sending header (AA BB + size {len(data)})...")
        header = bytes([0xAA, 0xBB]) + struct.pack("<I", len(data))
        ser.write(header)
        expect_ack(ser, "header")  # single ACK after MCU validates AA, BB, and size

        # send DATA one chunk at a time — MCU receives via polling HAL_UART_Receive
        # wait for ACK after each chunk before sending the next
        print("Sending DATA...")
        total    = len(data)
        offset   = 0
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
        time.sleep(0.05)
        # send CRC atomically — MCU receives this in one DMA transfer
        print(f"Sending CRC (0x{crc:08X})...")
        ser.write(struct.pack("<I", crc))
        expect_ack(ser, "CRC")  # ACK means CRC matched, NACK means mismatch

        # final OK/FAIL comes from main.c after UART_Receive returns RECEP_OK
        print("Waiting for final result...")
        result = wait_response(ser)
        if result == "OK":
            print("\nSuccess — firmware written to flash, MCU rebooting")
        else:
            print(f"\nFailed — MCU replied: {result}")
            sys.exit(1)

if __name__ == "__main__":
    # swap for: data = open("application.bin", "rb").read()
    data = bytes([0xAA, 0xBB, 0xCC, 0xDD] * 256)  # 1KB test pattern
    send_firmware(PORT, data)