#include "metadata.h"
#include "stm32f1xx_hal.h"

Flash_Status Metadata_Load(Metadata *m) {
    Metadata *stored = (Metadata *)METADATA_ADDRESS; // casting to Treat the bytes at memory address METADATA_ADDRESS as if they were a Metadata struct
    if (stored->magic == METADATA_MAGIC) {
        *m = *stored; // valid metadata found
    } else { // first boot ever, set defaults
        m->magic = METADATA_MAGIC;
        m->SLOTA_LATEST = true;
        m->bootcount = 0;
        return Metadata_Save(m);
    }
}

Flash_Status Metadata_Save(Metadata *m) {
    if(Flash_ErasePage(METADATA_ADDRESS) != FLASH_OK) {
        return FLASH_ERR_ERASE;
    }
    if(Flash_Write(METADATA_ADDRESS, (const uint8_t *)m, sizeof(Metadata)) != FLASH_OK) {
        return FLASH_ERR_WRITE;
    }
}

Flash_Status Metadata_IncrementBootCount(Metadata *m) {
    m->bootcount++;
    return Metadata_Save(m);
}

Flash_Status Metadata_UpdateAfterRecieve(Metadata *m, bool SLOTA_LATEST) {
    m->SLOTA_LATEST = SLOTA_LATEST ? 0 : 1;
    m->bootcount = 0;
    return Metadata_Save(m);
}