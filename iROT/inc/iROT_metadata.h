#ifndef IROT_METADATA_H
#define IROT_METADATA_H

#include <stdint.h>
#include <stdbool.h>

#define IROT_METADATA_ADDRESS 0x08001C00
#define IROT_METADATA_MAGIC 0xDEADBEEF

#define IROT_STATE_FIRST_BOOT     0xFFFFFFFFUL
#define IROT_STATE_PROVISIONED    0x00000000UL

typedef enum { // for iROT to survive 2 involutary ob forced resets during flash write
    IROT_WRP_IDLE = 0,
    IROT_WRP_DISABLING = 1,
    IROT_WRP_DISABLED = 2,
    IROT_WRP_ENABLING = 3,
    IROT_WRP_ENABLED = 4,
} IROT_WRP_State;

typedef struct __attribute__((packed)) {
    uint32_t magic; 
    uint32_t bl_update_pending;
    uint32_t bl_update_verified;
    uint32_t bl_bin_addr;
    uint32_t bl_bin_size;
    uint32_t wrp_state;
    uint32_t boot_state; // first boot or provisioned, first boot trusts unsigned bootloader blindly. 
    // uint32_t crc;
} IROT_Metadata;

void IROT_Metadata_Load(IROT_Metadata *m);
void IROT_Metadata_Save(IROT_Metadata *m);
void IROT_Metadata_ClearPending(IROT_Metadata *m);
void IROT_Metadata_SetPending(IROT_Metadata *m, uint32_t addr, uint32_t size);

#endif