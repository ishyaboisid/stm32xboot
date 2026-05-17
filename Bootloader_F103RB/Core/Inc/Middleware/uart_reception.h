/**
 * @file uart_reception.h
*/

#ifndef UART_RECEPTION_H
#define UART_RECEPTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "Middleware/metadata.h"

/*
Packet structure little endian:
0xAA | 0xBB | SIZE (4 bytes LE) | DATA (N bytes) | Hardware CRC32 (4 bytes LE) |
CRC32: polynomial 0x04C11DB7, init 0xFFFFFFFF
Data is padded to a mutiple of 4 bytes with 0xFF before CRC calculation (CRC computation is done on the whole 32-bit data word, and not byte per
byte - page 64 rm0008)
*/

#define RECEP_START_0 0xAAU
#define RECEP_START_1 0xBBU
#define RECEP_CHUNK_SIZE 1024U // bytes recieved per iteration matching one flash page

typedef enum {
    RECEP_OK = 0,
    RECEP_ERR_START,
    RECEP_ERR_SIZE,
    RECEP_ERR_RECV,
    RECEP_ERR_CRC32,
    RECEP_ERR_SIG,
    RECEP_ERR_VERSION,
} RECEP_STATUS;

RECEP_STATUS UART_Receive(uint8_t* received_header, Metadata *meta);

#endif