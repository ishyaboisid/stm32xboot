#include "pal/pal_crc.h"
#include "stm32/f1/system_init.h"

// src: CRC32: https://github.com/Steppeschool/stm32-custom-bootloader/tree/main

uint32_t crc_result;

void PAL_CRC_Reset(void) {
    __HAL_CRC_DR_RESET(&hcrc);
}

uint32_t PAL_CRC_Accumulate(uint8_t *data, size_t len) {
    size_t word_count = len / 4;
    size_t remainder = len % 4;

    for (size_t i = 0; i < word_count; i++) { // feed 32bit words msb first into CRC->DR
        uint32_t word = (uint32_t)data[i*4] << 24 | (uint32_t)data[i*4 + 1] << 16 | (uint32_t)data[i*4 + 2] << 8 | (uint32_t)data[i*4 + 3]; // MSB to LSB
        hcrc.Instance->DR = word;
    } 
    if (remainder > 0) { // for e.g. data = [0xAF, 0xBC, 0xD9, 0xC5, 0x70, 0x3E]. Remainder = 2 (i = 0, 1), therefore 
        uint32_t last = 0xFFFFFFFFU;
        /** @todo CALCULATE ACCUMULATE ALGO by hard to better understand */
        for (size_t i = 0; i < remainder; i++) {
            last = (last & ~(0xFFU << (24 - i*8))) | ((uint32_t)data[word_count*4 + i] << (24 - i*8));
        }
        hcrc.Instance->DR = last;
    }
    crc_result = hcrc.Instance->DR; 
    return crc_result;
}
uint32_t PAL_CRC_GetResult(void) {
    return crc_result;
}