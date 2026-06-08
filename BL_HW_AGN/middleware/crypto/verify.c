#include "verify.h"
#include "crypto/public_key.h"
#include "uECC/uECC.h"
#include "sha256/inc/sha256.h"

/**
 * @brief SHA-256 produces 32-byte digest of fw -> sign/verify hash (which cannot be reversed to recover fw)
uECC_verify checks that  signature was produced by private key corresponding to ECDSA_PUBLIC_KEY over hash.
If even one byte of the firmware changed after signing, the hash changes
and verification fails — the bootloader then refuses to flash.
signature is last 64 bytes of received data (32 bytes r + 32 bytes s)
curve used is uECC_secp256r1(): P-256 curve, matches keygen.py using SECP256R1
 * 
 */
int Verify_Firmware(uint8_t *data, size_t data_len, uint8_t *signature) {
    uint8_t hash[32];
    SHA256_CTX ctx;
    sha256_init(&ctx); // initialise SHA-256 internal state (set H0-H7 constants)
    sha256_update(&ctx, data, data_len); // feed firmware bytes into the hash — can be called in chunks
    sha256_final(&ctx, hash); // write 32-byte digest into hash[]

    return uECC_verify(ECDSA_PUBLIC_KEY, hash, sizeof(hash), signature, uECC_secp256r1());
}