#ifndef AES_CTR_H
#define AES_CTR_H

#include <stdint.h>
#include <stddef.h>

#define AES_KEY_SIZE 16U // bytes, shared secret between BL and Py
#define AES_IV_SIZE 16U // init vec nonce

extern uint8_t AES_KEY[AES_KEY_SIZE];

extern uint8_t AES_IV[AES_IV_SIZE];

#endif