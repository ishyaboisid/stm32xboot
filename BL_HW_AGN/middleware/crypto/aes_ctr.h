#ifndef AES_CTR_H
#define AES_CTR_H

#include <stdint.h>
#include <stddef.h>

#define AES_KEY_SIZE 16U // bytes, shared secret between BL and Py

extern uint8_t AES_KEY[AES_KEY_SIZE];

// AES_IV generated randomly by Python and recieved through DMA in uart_reception.c

#endif