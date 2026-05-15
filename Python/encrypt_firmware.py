from Crypto.Cipher import AES  # pip install pycryptodome
import struct
from pathlib import Path
import sys

KEY = bytes([
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
])

IV = bytes([
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
])

def encrypt_firmware(input_path: str, output_path: str):
    data = open(input_path, "rb").read()
    cipher = AES.new(KEY, AES.MODE_CTR, nonce=b'', initial_value=IV)
    encrypted = cipher.encrypt(data)
    open(output_path, "wb").write(encrypted)
    print(f"Encrypted {len(data)} bytes → {output_path}")

if __name__ == "__main__":
    encrypt_firmware(sys.argv[1], sys.argv[2])
# usage: python encrypt_firmware.py Application.bin Application.enc.bin