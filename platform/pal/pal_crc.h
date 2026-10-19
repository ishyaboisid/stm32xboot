#pragma once
#include <stdint.h>
#include <stddef.h>

void PAL_CRC_Reset(void);
uint32_t PAL_CRC_Accumulate(uint8_t *data, size_t len);
uint32_t PAL_CRC_GetResult(void);