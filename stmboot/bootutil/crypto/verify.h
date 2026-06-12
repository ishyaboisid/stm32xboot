#ifndef VERIFY_H
#define VERIFY_H

#include <stdint.h>
#include <stddef.h>

int Verify_Firmware(uint8_t *data, size_t data_len, uint8_t *signature);

#endif