#include "img_security.h"
#include "crypto/verify.h"
#include "crypto/aes_ctr.h"
#include "aes/inc/aes.h"
#include "platform.h"

extern CRC_HandleTypeDef hcrc;

static struct AES_ctx aes_ctx;
uint32_t crc_result_sec;

SEC_Status security_begin(SEC_Ctx *ctx, uint32_t fw_size, const uint8_t iv[16]) {
    ctx->total_size = fw_size;
    ctx->bytes_processed = 0;
    
    PAL_CRC_Reset();
    AES_init_ctx_iv(&aes_ctx, AES_KEY, iv);

    return SEC_OK;
}

SEC_Status security_processChunk(SEC_Ctx *ctx, uint8_t *chunk, uint32_t chunk_len, uint16_t uart_size) {
    AES_CTR_xcrypt_buffer(&aes_ctx, chunk, chunk_len); // decrypt before crc & flash write, uart_size is chunk len. todo, do i need 2 variables??
    crc_result_sec = PAL_CRC_Accumulate(chunk, chunk_len);

    ctx->bytes_processed += chunk_len;
    return SEC_OK;
}

SEC_Status security_verify_crc(SEC_Ctx *ctx, uint32_t recieved_crc) {
    // uint32_t calculated_crc = PAL_CRC_GetResult(); // todo idk why this doesn't work
    if (crc_result_sec != recieved_crc) return SEC_ERR_CRC;
    return SEC_OK;
}

SEC_Status security_verify_sig(SEC_Ctx *ctx, uint32_t slot_start, int ptype) {
    uint8_t signature[64];
    uint32_t sig_addr = slot_start + ctx->total_size - 64U; // signature is the last 64 bytes written to flash
    for (int i = 0; i < 64; i++) {
        signature[i] = *(volatile uint8_t *)(sig_addr + i); 
    }
    if (ptype == 0) {
        if (!Verify_Firmware((uint8_t *)slot_start, ctx->total_size - 64U, signature)) { // returns uECC_verify which return int 1 if signature valid and 0 if not
            return SEC_ERR_SIG; 
        }
    } else if (ptype == 1) {
        if (!Verify_BL((uint8_t *)slot_start, ctx->total_size - 64U, signature)) { // returns uECC_verify which return int 1 if signature valid and 0 if not
            return SEC_ERR_SIG; 
        }
    } else return SEC_ERR_SIG; 
    return SEC_OK;
}

bool Reverify_Signature(Metadata *meta, uint32_t slot_addr) { // for use with no security context 
    uint8_t signature[64];
    uint32_t sig_addr = slot_addr + meta->fw_size - 64U;
    for (int i = 0; i < 64; i++) {
        signature[i] = *(volatile uint8_t *)(sig_addr + i);
    }
    return Verify_Firmware((uint8_t *)slot_addr, meta->fw_size - 64U, signature);
}