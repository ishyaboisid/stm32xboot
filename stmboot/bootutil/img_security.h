#ifndef IMG_SECURITY_H
#define IMG_SECURITY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "metadata.h"

typedef enum {
    SEC_OK = 0,
    SEC_ERR_CRC,
    SEC_ERR_SIG,
} SEC_Status;

typedef struct {
    uint32_t total_size;
    uint32_t bytes_processed; 
} SEC_Ctx;

SEC_Status security_begin(SEC_Ctx *ctx, uint32_t fw_size, const uint8_t iv[16]); // initialise AES context with received IV, reset CRC peripheral
SEC_Status security_processChunk(SEC_Ctx *ctx, uint8_t *chunk, uint32_t chunk_len, uint16_t uart_size); // decrypt chunk, accumulate crc, chunk ptr is modified so caller writes from same buffer
SEC_Status security_verify_crc(SEC_Ctx *ctx, uint32_t recieved_crc);
SEC_Status security_verify_sig(SEC_Ctx *ctx, uint32_t slot_start, int ptype); // verify crc, verify ecdsa sig based on update type
bool Reverify_Signature(Metadata *meta, uint32_t slot_addr);

#endif