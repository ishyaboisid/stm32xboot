#include "metadata.h"
#include <stdint.h>

static uint32_t compute_metadata_crc(Metadata *m) {
    PAL_CRC_Reset();
    PAL_CRC_Accumulate((uint8_t *)m, offsetof(Metadata, crc));
    return PAL_CRC_GetResult();
}

static uintptr_t return_Active_Metadata_Address() {
    Metadata *stored1 = (Metadata *)METADATA_ADDRESS_SLOT1;
    Metadata *stored2 = (Metadata *)METADATA_ADDRESS_SLOT2;

    bool v1 = (stored1->magic == METADATA_MAGIC) && stored1->sequence != 0xFFFFFFFF; // if magic is valid + sequence has been initalized (not garbage)
    bool v2 = (stored2->magic == METADATA_MAGIC) && stored2->sequence != 0xFFFFFFFF;

    if (v1 && !v2) return METADATA_ADDRESS_SLOT1;
    if (!v1 && v2) return METADATA_ADDRESS_SLOT2;

    if (v1 && v2) {
        if (stored1->sequence > stored2->sequence) { return METADATA_ADDRESS_SLOT1; }
        if (stored1->sequence < stored2->sequence) { return METADATA_ADDRESS_SLOT2; }
    }

    return METADATA_ADDRESS_SLOT1;
}

static uintptr_t return_Inactive_Metadata_Address() {
    if (return_Active_Metadata_Address() == METADATA_ADDRESS_SLOT1) {
        return METADATA_ADDRESS_SLOT2;
    } else {
        return METADATA_ADDRESS_SLOT1;
    }
}


PAL_Flash_StatusTypeDef Metadata_Load(Metadata *m) {
    Metadata *stored = (Metadata *)return_Active_Metadata_Address(); // casting to Treat the bytes at memory address METADATA_ADDRESS as if they were a Metadata struct
    if (stored->magic == METADATA_MAGIC) {
        if (compute_metadata_crc(stored) == stored->crc) {
            *m = *stored; // valid metadata found
            return FLASH_OK;
        } else { // todo redundant code remove
            m->magic = METADATA_MAGIC;
            m->SLOTA_LATEST = true;
            m->bootcount = 0;
            m->runtime_fault_count = 0;
            m->FW_VER_MAJOR = 0x0;
            m->FW_VER_MINOR = 0x0;
            m->image_state = IMG_STATE_NONE;
            m->sequence = 1;
            m->crc = compute_metadata_crc(m);
            return Metadata_Save(m); 
        }
    } else { // first boot ever, set defaults
        m->magic = METADATA_MAGIC;
        m->SLOTA_LATEST = true;
        m->bootcount = 0;
        m->runtime_fault_count = 0;
        m->FW_VER_MAJOR = 0x0;
        m->FW_VER_MINOR = 0x0;
        m->image_state = IMG_STATE_NONE;
        m->sequence = 1;
        m->crc = compute_metadata_crc(m);
        return Metadata_Save(m);
    }
}

PAL_Flash_StatusTypeDef Metadata_Save(Metadata *m) {
    uintptr_t target = return_Inactive_Metadata_Address(); 
    Metadata temp = *m;
    temp.sequence++;
    temp.crc = compute_metadata_crc(&temp);

    if(PAL_Flash_ErasePage(target) != FLASH_OK) {
        return FLASH_ERR_ERASE;
    }

    if(PAL_Flash_Write(target, (const uint8_t *)&temp, sizeof(Metadata)) != FLASH_OK) {
        return FLASH_ERR_WRITE;
    }

    PAL_CRC_Reset();
    return FLASH_OK;
}

PAL_Flash_StatusTypeDef Metadata_IncrementBootCount(Metadata *m) {
    Metadata *stored = (Metadata *)return_Active_Metadata_Address();
    if (stored->magic == METADATA_MAGIC) {
        *m = *stored;
        m->bootcount++;
        return Metadata_Save(m);
    }
    return FLASH_ERR_INVALID_ADDR;
}

PAL_Flash_StatusTypeDef Metadata_UpdateAfterRecieve(Metadata *m, bool SLOTA_LATEST) {
   Metadata *stored = (Metadata *)return_Active_Metadata_Address();
    if (stored->magic == METADATA_MAGIC) {
        *m = *stored;
        m->SLOTA_LATEST = SLOTA_LATEST ? 0 : 1; 
        m->bootcount = 0;
        m->image_state = IMG_STATE_PENDING; // not confirmed, will be confirmed by app
        return Metadata_Save(m);
    }
    return FLASH_ERR_INVALID_ADDR;
}