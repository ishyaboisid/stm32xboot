#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include "platform.h"

#define METADATA_MAGIC 0xDEADBEEFU // start of metadata
#define METADATA_SLOT_SIZE 0x400UL // 1kb

// rollback
#define BOOT_COUNT_MAX 3U 
#define RUNTIME_BOOT_COUNT_MAX 10U

typedef enum {
    BL_STATE_IDLE = 0x0, // no writing in prog
    BL_STATE_ERASING = 0x1, // erase started not completed
    BL_STATE_WRITING = 0x2, // write started not completed
} BLState; // todo change to systemstate

typedef enum {
    FWU_STATE_IDLE = 0x0,
    FWU_STATE_START = 0x1,
    FWU_STATE_ERASEDSLOT = 0x2,
    FWU_STATE_WRITECOMPLETE = 0x3,
    FWU_STATE_CRCVERIFIED = 0x4,
    FWU_STATE_ECCVERIFIED = 0x5,
} FWUState; 

typedef enum {
    IMG_STATE_NONE = 0x0, // no fw erased flash
    IMG_STATE_PENDING = 0x1, // fw recieved, not booted
    IMG_STATE_TRIAL = 0x2, // booted not confirmed
    IMG_STATE_HEALTHY = 0x3, 
    IMG_STATE_REVERTED = 0x4, // BOOT_COUNT_MAX exceeded, rolled back
} ImageState;

typedef struct __attribute__((packed)) { // using fixed-width integer types for stored state values rather than relying on enum size.
    uint32_t magic;
    uint32_t SLOTA_LATEST;
    uint32_t bootcount; // increment every boot, clear by successful firmware
    uint8_t  FW_VER_MAJOR;
    uint8_t  FW_VER_MINOR;
    uint8_t  BL_VER_MAJOR;
    uint8_t  BL_VER_MINOR;
    uint16_t _pad;
    uint32_t image_state;
    uint32_t runtime_fault_count;
    uint32_t sequence; // monotonically increasing — higher = newer
    uint8_t iv[16];
    uint32_t fw_size;
    uint32_t bl_state;
    uint32_t fwu_state;
    uint32_t bl_update_requested;
    uint32_t crc; 
} Metadata;

/*
magic               4
SLOTA_LATEST        4
bootcount           4
FW_VER_MAJOR        1
FW_VER_MINOR        1
BL_VER_MAJOR        1
BL_VER_MINOR        1
_pad                2
image_state         4
runtime_fault_count 4
sequence            4
iv                  16
fw_size             4
bl_state            4
fwu_state           4
bl_update_requested 4
crc                 4
----------------------
TOTAL              66 bytes
*/
_Static_assert(sizeof(Metadata) == 66, "Metadata size changed");
_Static_assert(sizeof(Metadata) <= 1024, "Metadata does not fit in 1 KB");

PAL_Flash_StatusTypeDef Metadata_Load(Metadata *m); // reads from flash, writes defaults
PAL_Flash_StatusTypeDef Metadata_Save(Metadata *m); // erases metadata page & writes new
PAL_Flash_StatusTypeDef Metadata_IncrementBootCount(Metadata *m);
PAL_Flash_StatusTypeDef Metadata_UpdateAfterRecieve(Metadata *m, bool SLOTA_LATEST);

#endif