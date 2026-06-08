from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import hashes, serialization
import sys

def sign_firmware(private_key_path: str, firmware_path: str, output_path: str):
    key = serialization.load_pem_private_key(
        open(private_key_path, "rb").read(), password=None
    )

    firmware = open(firmware_path, "rb").read()

    # sign with SHA-256 — produces DER-encoded signature
    # micro-ecc expects raw 64-byte signature (32 r + 32 s)
    from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
    sig_der = key.sign(firmware, ec.ECDSA(hashes.SHA256()))
    r, s = decode_dss_signature(sig_der)

    # pad r and s to 32 bytes each
    sig_raw = r.to_bytes(32, "big") + s.to_bytes(32, "big")

    # append signature to firmware — bootloader reads last 64 bytes as signature
    open(output_path, "wb").write(firmware + sig_raw)
    print(f"Signed {len(firmware)} bytes, signature appended → {output_path}")
    print(f"Total: {len(firmware) + 64} bytes")

if __name__ == "__main__":
    sign_firmware(sys.argv[1], sys.argv[2], sys.argv[3])