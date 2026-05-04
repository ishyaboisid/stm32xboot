#include "flash.h"
#include "stm32f1xx_hal.h"

/**
 * @brief private fn checks slotA start -< addr > slotB end
 * @return int 1 if valid addr
 */
static int is_valid_slot_address(uint32_t addr) { // private fn
    return (addr >= SLOTA_START_ADDRESS) && (addr < (SLOTB_START_ADDRESS + (SLOTB_NUM_PAGES * FLASH_PAGE_SIZE_BL)));
}

Flash_Status Flash_ErasePage(uint32_t page_address) {
    if (!is_valid_slot_address(page_address)) return FLASH_ERR_INVALID_ADDR;
    if (page_address % FLASH_PAGE_SIZE_BL != 0) return FLASH_ERR_INVALID_ADDR;

    FLASH_EraseInitTypeDef erase = {
       .TypeErase = FLASH_TYPEERASE_PAGES,
       .Banks = FLASH_BANK_1, // f103rb only has single bank
       .PageAddress = page_address,
       .NbPages = 1U, 
    };

    uint32_t page_error = 0;
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &page_error);
    HAL_FLASH_Lock();

    if (status != HAL_OK || page_error != 0xFFFFFFFF) return FLASH_ERR_ERASE; // 0xFFFFFFFF means that all the pages have been correctly erased
    return FLASH_OK;
}

Flash_Status Flash_EraseSlot(uint32_t slot_start, uint32_t num_pages) {
    for (uint32_t i = 0; i < num_pages; i++) {
        Flash_Status s = Flash_ErasePage(slot_start + (i * FLASH_PAGE_SIZE_BL));
        if (s != FLASH_OK) return s;
    }
    return FLASH_OK;
}

/*
STM32F103 flash controller can only write in 16-bit halfwords - cannot write a single byte to flash
Every write operation must be exactly 2 bytes aligned to a 2-byte boundary.
If binary is:
    byte 0: 0xDE
    byte 1: 0xAD
    byte 2: 0xBE
    byte 3: 0xEF
Write in two halfword operations:
    i=0: write 0xADDE to address+0
    i=2: write 0xEFBE to address+2
STM32 is little-endian, so src[i] | (src[i+1] << 8) puts the lower-indexed byte in the low position
The odd-length case is why  padding exists. If len = 5:
    i=0: write src[0] | src[1] << 8   — fine
    i=2: write src[2] | src[3] << 8   — fine
    i=4: write src[4] | 0xFF << 8     — pad the missing 6th byte with 0xFF
Without pad reading src[5] is out of bounds. 0xFF is safe as erased flash is already 0xFF
*/
Flash_Status Flash_Write(uint32_t dest, const uint8_t *src, size_t len) {
   if (!is_valid_slot_address(dest)) return FLASH_ERR_INVALID_ADDR;
   
   HAL_FLASH_Unlock();

   for (size_t i = 0; i < len; i+=2) {
    uint8_t lo = src[i]; 
    uint8_t hi = (i + 1 < len) ? src[i + 1] : 0xFF; // padding if len is odd; ternary operator
    uint16_t halfword = (uint16_t)lo | ((uint16_t)hi << 8); // move to left by 8 - little endian stm32

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, dest + i, halfword) != HAL_OK) { // every dest + i is halfword incrementation 
        HAL_FLASH_Lock();
        return FLASH_ERR_WRITE;    
    }
   }

   HAL_FLASH_Lock();
   return FLASH_OK;
}

Flash_Status Flash_Verify(uint32_t dest, const uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (*(volatile uint8_t *)(dest + i) != src[i]) return FLASH_ERR_VERIFY; // go to pointer address dest + i and read 1 byte (8 bits) from it
    }
    return FLASH_OK;
}