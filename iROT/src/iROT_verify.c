#include "iROT_verify.h"
#include "iROT_public_key.h"
#include "sha256.h"
#include "uECC.h"
#include <string.h>

bool iROT_VerifySTMBoot(uint32_t addr, uint32_t size) {
    uint8_t signature[64];
    uint32_t data_len = size - 64U;
    uint32_t sig_addr = addr + data_len;
    // uint32_t sig_addr = addr - 64U; 
    for (int i = 0; i < 64; i++) {
        signature[i] = *(volatile uint8_t *)(sig_addr + i); 
    }
    uint8_t hash[32];
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (uint8_t *)addr, size - 64U); 
    sha256_final(&ctx, hash);

    return uECC_verify(IROT_PUBLIC_KEY, hash, sizeof(hash), signature, uECC_secp256r1());
}