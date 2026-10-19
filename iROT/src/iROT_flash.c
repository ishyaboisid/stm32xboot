#include "iROT_flash.h"
#include "iROT_verify.h"
#include "iROT_metadata.h" // todo uint32_t wrp_state in IROT_Metadata, handle each case here
#include "../../platform/pal/pal_flash.h"
#include "../../stmboot/drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h"
#include <string.h>

// static int disable_stmboot_write_protect(IROT_Metadata *m) {
//     m->wrp_state = IROT_WRP_DISABLING; 
//     IROT_Metadata_Save(m);

//     FLASH_OBProgramInitTypeDef ob = {0};
//     HAL_FLASHEx_OBGetConfig(&ob);

//     HAL_FLASH_OB_Unlock(); // todo maybe implement lock unlock in PAL?
//     HAL_FLASH_Unlock();

//     ob.OptionType = OPTIONBYTE_WRP;
//     ob.WRPState = OB_WRPSTATE_DISABLE;
//     ob.WRPPage = // todo add page ranges

//     HAL_StatusTypeDef s = HAL_FLASHEx_OBProgram(&ob);
//     HAL_FLASH_OB_Lock();
//     HAL_FLASH_Lock();

//     if (s == HAL_OK) {
//         m->wrp_state = IROT_WRP_DISABLED; 
//         IROT_Metadata_Save(m);
//         HAL_FLASH_OB_Launch();
//     }
//     return s == 1; 
// }

// static int enable_stmboot_write_protect(IROT_Metadata *m) {
//     m->wrp_state = IROT_WRP_ENABLING; 
//     IROT_Metadata_Save(m);
//     FLASH_OBProgramInitTypeDef ob = {0};

//     HAL_FLASH_OB_Unlock();
//     HAL_FLASH_Unlock();

//     ob.OptionType = OPTIONBYTE_WRP;
//     ob.WRPState = OB_WRPSTATE_ENABLE;
//     ob.WRPPage = // todo add page ranges

//     HAL_StatusTypeDef s = HAL_FLASHEx_OBProgram(&ob);
//     HAL_FLASH_OB_Lock();
//     HAL_FLASH_Lock();

//     if (s == HAL_OK) {
//         m->wrp_state = IROT_WRP_ENABLED; 
//         IROT_Metadata_Save(m);
//         HAL_FLASH_OB_Launch();
//     }
//     return s == 1;  
// }

int iROT_Flash_InstallSTMBoot(IROT_Metadata *m, uint32_t size) {
    // disable_stmboot_write_protect();

    for (uint32_t i = 0; i < STMBOOT_NUM_PAGES; i++) { // erase
        if (PAL_Flash_ErasePage(STMBOOT_ADDRESS + i * 1024U) != FLASH_OK) {
            // enable_stmboot_write_protect(); // relock on failure
            return 1;
        }
    }

    uint32_t remaining_data = size; 
    uint32_t write_addr = STMBOOT_ADDRESS;


    const uint8_t *src = (const uint8_t *)m->bl_bin_addr;
    while (remaining_data > 0) {
        uint32_t chunk_len = remaining_data < 1024 ? remaining_data : 1024;

        if (PAL_Flash_Write(write_addr, src, chunk_len) != FLASH_OK) { // chunk isn't declared, taking directly from meta staged addr
            // enable_stmboot_write_protect();
            return 1;
        }
        src += chunk_len;
        write_addr += chunk_len;
        remaining_data -= chunk_len;
    }

    if(!iROT_VerifySTMBoot(STMBOOT_ADDRESS, size)) {
        // enable_stmboot_write_protect();
        return 1;
    }

    // enable_stmboot_write_protect();

    m->bl_update_pending = 0;
    IROT_Metadata_Save(m);
    return 0;

}