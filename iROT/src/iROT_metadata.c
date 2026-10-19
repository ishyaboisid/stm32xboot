#include "iROT_metadata.h"
// todo decision: should I pull pal->HAL or directly implement flash writing for size?
#include "../../platform/pal/pal_flash.h" 
#include "../../platform/pal/pal_crc.h" 
#include <string.h>

// static uint32_t compute_metadata_crc(IROT_Metadata *m) { // todo crc
//     PAL_CRC_Reset();
//     PAL_CRC_Accumulate((uint8_t *)m, offsetof(IROT_Metadata, crc));
//     return PAL_CRC_GetResult();
// }

void IROT_Metadata_Load(IROT_Metadata *m) { // todo add metadata crc check
    IROT_Metadata *stored = (const IROT_Metadata *)IROT_METADATA_ADDRESS;
    if ((stored->magic == IROT_METADATA_MAGIC)) { // && (compute_metadata_crc(stored) == stored->crc)) {
        *m = *stored;
        return;
    }
    // first boot / invalid page - defaults
    m->magic = IROT_METADATA_MAGIC;
    m->bl_update_pending = 0;
    m->bl_update_verified = 0;
    m->bl_bin_addr = 0x0;
    m->bl_bin_size = 0;
    m->boot_state = 0xFFFFFFFFUL;
    // m->crc = compute_metadata_crc(m);
    IROT_Metadata_Save(m);
    return;
}

void IROT_Metadata_Save(IROT_Metadata *m) { // todo disable enable write protection??, recompute and store crc
    IROT_Metadata temp = *m;
    // temp.crc = compute_metadata_crc(&temp);
    PAL_Flash_ErasePage(IROT_METADATA_ADDRESS);
    volatile int status = PAL_Flash_Write(IROT_METADATA_ADDRESS, (const uint8_t *)&temp, sizeof(IROT_Metadata));
    if (status != 0) return;
    return;
}

void IROT_Metadata_ClearPending(IROT_Metadata *m) {
    m->bl_update_pending = 0;
    m->bl_bin_addr = 0x0;
    m->bl_bin_size = 0;
    IROT_Metadata_Save(m);
}

void IROT_Metadata_SetPending(IROT_Metadata *m, uint32_t addr, uint32_t size) {
    m->bl_update_pending = 1;
    m->wrp_state = 0;
    m->bl_bin_addr = addr;
    m->bl_bin_size = size;
    m->boot_state = 0x00000000UL; // set to provisioned here so we can verify it? idk todo
    IROT_Metadata_Save(m);
}