#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum {
    FLASH_OK = 0,
    FLASH_ERR_INVALID_ADDR,
    FLASH_ERR_ERASE,
    FLASH_ERR_WRITE,
} PAL_Flash_StatusTypeDef;

PAL_Flash_StatusTypeDef PAL_Flash_ErasePage(uint32_t page_address);
PAL_Flash_StatusTypeDef PAL_Flash_EraseSlot(uint32_t slot_start, uint32_t num_pages);
PAL_Flash_StatusTypeDef PAL_Flash_Write(uint32_t dest, const uint8_t *src, size_t len);