# todo also create a .sh for this
import serial
import struct
import sys
from pathlib import Path
import time
import argparse

import encrypt_firmware
import sign_firmware


CHUNK_SIZE = 1024

# Bootloader binary
STMBOOT_BIN = Path("../stmboot/build/f103rb/debug/Bootloader.bin")

# Output files
SIGNED_PATH = Path("STMBoot.signed.bin")
ENCRYPTED_PATH = Path("STMBoot.enc.bin")

# Signing key
IROT_PRIVATE_KEY = "iROT_private_key.pem"

# Bootloader version being installed
BL_VERSION = (1, 1)

PORT       = "/dev/tty.usbmodem103"
BAUDRATE   = 115200

def compute_crc32_stm32(data: bytes) -> int:
    """
    Match the STM32 CRC peripheral configuration used by the bootloader.

    CRC:
        Initial value = 0xFFFFFFFF
        Polynomial    = 0x04C11DB7
        Input/output  = non-reflected

    Data is padded with 0xFF to a 32-bit word boundary.
    """
    crc = 0xFFFFFFFF

    remainder = len(data) % 4
    if remainder:
        data = data + b"\xFF" * (4 - remainder)

    for i in range(0, len(data), 4):
        word = struct.unpack(">I", data[i:i + 4])[0]

        crc ^= word

        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF

    return crc


def wait_response(ser) -> str:
    line = ser.readline().decode(errors="replace").strip()

    if line:
        print(f"  MCU: {line}")

    return line


def waiting_for_ready(ser, timeout_s=10):
    print("\nWaiting for BL_READY...")

    start = time.monotonic()

    while True:
        if time.monotonic() - start > timeout_s:
            raise TimeoutError("BL_READY timeout")

        line = ser.readline().decode(errors="replace").strip()

        if not line:
            continue

        print(f"  MCU: {line}")

        if line == "BL_READY":
            return


def expect_ack(ser, context, timeout_s=5):
    """
    Wait for ACK from the bootloader.

    C bootloader sends:
        "ACK\\r\\n"
        "NACK\\r\\n"
    """

    start = time.monotonic()

    while True:
        # if time.monotonic() - start > timeout_s:
        #     raise TimeoutError(f"ACK timeout at {context}")

        line = ser.readline().decode(errors="replace").strip()

        if not line:
            continue

        print(f"  MCU: {line}")

        if line == "ACK":
            return

        if line == "NACK":
            raise RuntimeError(f"NACK received at {context}")

        raise RuntimeError(
            f"Unexpected response at {context}: {line}"
        )


def send_bl_update(ser):
    # ------------------------------------------------------------------
    # 1. HEADER
    #
    # C expects:
    #   received_header[0] = BL_RECEP_START_0 = 0xCC
    #   received_header[1] = BL_RECEP_START_1 = 0xDD
    #   received_header[2] = major version
    #   received_header[3] = minor version
    # ------------------------------------------------------------------

    print(
        f"Sending BL header "
        f"(0xCC 0xDD, version {BL_VERSION[0]}.{BL_VERSION[1]})..."
    )

    header = bytes([
        0xCC,
        0xDD,
        BL_VERSION[0],
        BL_VERSION[1],
    ])

    ser.write(header)

    # C sends either:
    #   "A\r\n"
    # or:
    #   "B\r\n"
    #
    # depending on which slot is currently NOT latest.
    print("Waiting for slot assignment...")

    slot = None

    while True:
        line = ser.readline().decode(errors="replace").strip()

        if not line:
            continue

        print(f"  MCU: {line}")

        if line in ("A", "B"):
            slot = line
            break

        if line == "NACK":
            raise RuntimeError("Bootloader rejected header")

    # C then sends ACK after selecting/erasing the slot.
    expect_ack(ser, "header")

    print(f"Slot {slot} assigned")

    # ------------------------------------------------------------------
    # 2. SIGN BOOTLOADER
    #
    # The signature is appended to the plaintext binary.
    #
    # Result:
    #
    #   [ bootloader payload ][ 64-byte signature ]
    # ------------------------------------------------------------------

    print("\nSigning bootloader...")

    sign_firmware.sign_firmware(
        IROT_PRIVATE_KEY,
        str(STMBOOT_BIN),
        str(SIGNED_PATH),
    )

    signed_plaintext = SIGNED_PATH.read_bytes()

    print(f"Signed firmware: {len(signed_plaintext)} bytes")

    # ------------------------------------------------------------------
    # 3. ENCRYPT SIGNED BOOTLOADER
    #
    # encrypt_firmware returns the IV.
    #
    # The C bootloader receives this IV and initializes:
    #
    #   AES_init_ctx_iv(&ctx, AES_KEY, meta->iv);
    # ------------------------------------------------------------------

    print("Encrypting signed bootloader...")

    iv = encrypt_firmware.encrypt_firmware(
        str(SIGNED_PATH),
        str(ENCRYPTED_PATH),
    )

    encrypted_data = ENCRYPTED_PATH.read_bytes()

    if len(encrypted_data) != len(signed_plaintext):
        raise RuntimeError(
            "Encrypted firmware size differs from signed firmware size"
        )

    print(f"Encrypted firmware: {len(encrypted_data)} bytes")
    print(f"IV: {iv.hex()}")

    # ------------------------------------------------------------------
    # 4. CRC
    #
    # IMPORTANT:
    #
    # The C bootloader decrypts each chunk BEFORE calculating CRC:
    #
    #   AES_CTR_xcrypt_buffer(...)
    #   PAL_CRC_Accumulate(chunk, chunk_len)
    #
    # Therefore CRC must be calculated over signed plaintext,
    # NOT encrypted_data.
    # ------------------------------------------------------------------

    crc = compute_crc32_stm32(signed_plaintext)

    print(f"CRC32: 0x{crc:08X}")

    # ------------------------------------------------------------------
    # 5. SIZE
    #
    # C:
    #
    #   PAL_UARTEx_Receive_DMA(size_buf, sizeof(size_buf));
    #
    #   uint32_t total_len =
    #       size_buf[0]
    #       | size_buf[1] << 8
    #       | size_buf[2] << 16
    #       | size_buf[3] << 24;
    #
    # Therefore little-endian uint32.
    # ------------------------------------------------------------------

    print(f"\nSending SIZE ({len(encrypted_data)} bytes)...")

    ser.write(struct.pack("<I", len(encrypted_data)))

    expect_ack(ser, "size")

    # ------------------------------------------------------------------
    # 6. IV
    #
    # C expects exactly 16 bytes:
    #
    #   PAL_UARTEx_Receive_DMA(meta->iv, sizeof(iv));
    #
    # Then:
    #
    #   AES_init_ctx_iv(&ctx, AES_KEY, meta->iv);
    # ------------------------------------------------------------------

    if len(iv) != 16:
        raise RuntimeError(
            f"Invalid IV length: expected 16 bytes, got {len(iv)}"
        )

    print("Sending IV...")

    ser.write(iv)

    expect_ack(ser, "IV")
    # time.sleep(0.05)

    # ------------------------------------------------------------------
    # 7. DATA
    #
    # C receives:
    #
    #   chunk_len = min(remaining_data, UART_RECEP_CHUNK_SIZE)
    #
    # So CHUNK_SIZE here must match UART_RECEP_CHUNK_SIZE in C.
    #
    # The transmitted data is ENCRYPTED.
    # ------------------------------------------------------------------

    print("\nSending encrypted DATA...")

    total = len(encrypted_data)
    offset = 0
    chunk_num = 0

    while offset < total:
        chunk = encrypted_data[
            offset:offset + CHUNK_SIZE
        ]
        ser.write(chunk)
        
        expect_ack(
            ser,
            f"chunk {chunk_num}"
        )

        offset += len(chunk)
        chunk_num += 1

        progress = (offset / total) * 100

        print(
            f"  Progress: "
            f"{offset}/{total} bytes "
            f"({progress:.1f}%)",
            end="\r",
        )

    print()

    # C sends one additional ACK after the entire data stream:
    #
    #   send_ack(); // for data as a whole
    #
    expect_ack(ser, "whole data")

    # ------------------------------------------------------------------
    # 8. CRC
    #
    # C expects:
    #
    #   uint8_t crc_buf[4];
    #   PAL_UART_Receive(crc_buf, 4, ...);
    #
    # Little endian.
    # ------------------------------------------------------------------

    print(f"Sending CRC (0x{crc:08X})...")

    ser.write(struct.pack("<I", crc))

    # C sends ACK only if CRC matches:
    #
    #   if (calculated_crc != received_crc_value)
    #       return RECEP_ERR_CRC32;
    #
    #   send_ack();
    #
    expect_ack(ser, "CRC")

    # ------------------------------------------------------------------
    # 9. ECC / SIGNATURE VERIFICATION
    #
    # C now extracts the last 64 bytes from flash and calls:
    #
    #   Verify_BL(...);
    #
    # On success:
    #
    #   send_ack();
    #
    # There is currently NO "OK\\r\\n" in the C code.
    # ------------------------------------------------------------------

    print("Waiting for signature verification...")

    expect_ack(ser, "signature")

    print(
        f"\nSuccess — bootloader v{BL_VERSION[0]}.{BL_VERSION[1]} "
        f"written to slot {slot}."
    )

    print(f"  Slot       : {slot}")
    print(f"  Size       : {len(signed_plaintext)} bytes")
    print(f"  CRC32      : 0x{crc:08X}")
    print(f"  IV         : {iv.hex()}")

    return


if __name__ == "__main__":

    while True:
        try:
            # ----------------------------------------------------------
            # Wait until the device is physically connected.
            # ----------------------------------------------------------

            ser = None

            while ser is None:
                try:
                    ser = serial.Serial(
                        PORT,
                        BAUDRATE,
                        timeout=0.2,
                    )

                    print(f"Connected to {PORT}")

                except serial.SerialException:
                    print("Waiting for device...")
                    time.sleep(1)

            with ser:
                waiting_for_ready(
                    ser,
                    timeout_s=10000000,
                )

                send_bl_update(ser)

            # Success — exit retry loop.
            break

        except (serial.SerialException, OSError) as e:
            print(f"\nConnection lost: {e}")
            print("Reconnecting...\n")
            time.sleep(1)
            continue

        except TimeoutError as e:
            print(f"\nTimeout: {e}")
            print("Retrying from the beginning...\n")
            time.sleep(1)
            continue

        except RuntimeError as e:
            print(f"\nProtocol error: {e}")
            print("Retrying from the beginning...\n")
            time.sleep(1)
            continue