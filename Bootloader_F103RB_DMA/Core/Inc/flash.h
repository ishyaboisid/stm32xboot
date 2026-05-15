#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>
#include <stddef.h>

#define FLASH_PAGE_SIZE_BL 1024U // STM32F103: 1KB (1024b) pages
#define SLOTA_START_ADDRESS 0x08006000 // 0x08004000U
#define SLOT_NUM_PAGES 48U
#define SLOTB_START_ADDRESS 0x08012000 // 0x08010000U
#define FLASH_EOF 0x08020000 // end of flash

typedef enum {
    FLASH_OK = 0,
    FLASH_ERR_INVALID_ADDR,
    FLASH_ERR_ERASE,
    FLASH_ERR_WRITE,
} Flash_Status;

Flash_Status Flash_ErasePage(uint32_t page_address);
Flash_Status Flash_EraseSlot(uint32_t slot_start, uint32_t num_pages);
Flash_Status Flash_Write(uint32_t dest, const uint8_t *src, size_t len); // size_t represents size/count of bytes in memory (on cortex m3 size_t = 32bits anyway), but avoids implicit conversion warning when sizeof()

#endif