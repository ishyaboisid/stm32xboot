#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include "platform.h"
#include "board_config.h"

#define METADATA_MAGIC 0xDEADBEEFU // start of metadata
#define METADATA_SLOT_SIZE 0x1000 // 4kb

// rollback
#define BOOT_COUNT_MAX 3U 
#define RUNTIME_BOOT_COUNT_MAX 10U

typedef enum {
    WRITE_STATE_IDLE = 0x0, // no writing in prog
    WRITE_STATE_ERASING = 0x1, // erase started not completed
    WRITE_STATE_WRITING = 0x2, // write started not completed
    WRITE_STATE_COMPLETE = 0x3, // all bytes written
} WriteState;

typedef enum {
    IMG_STATE_NONE = 0x0, // no fw erased flash
    IMG_STATE_PENDING = 0x1, // fw recieved, not booted
    IMG_STATE_TRIAL = 0x2, // booted not confirmed
    IMG_STATE_HEALTHY = 0x3, 
    IMG_STATE_REVERTED = 0x4, // BOOT_COUNT_MAX exceeded, rolled back
} ImageState;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t SLOTA_LATEST;
    uint32_t bootcount; // increment every boot, clear by successful firmware
    uint8_t  FW_VER_MAJOR;
    uint8_t  FW_VER_MINOR;
    uint16_t _pad;
    uint32_t image_state;
    uint32_t runtime_fault_count;
    uint32_t sequence; // monotonically increasing — higher = newer
    uint8_t iv[16];
    uint32_t write_state;
    uint32_t crc; 
} Metadata;

// _Static_assert(sizeof(Metadata) == 16, "Metadata struct size not 16b"); // todo is failing

PAL_Flash_StatusTypeDef Metadata_Load(Metadata *m); // reads from flash, writes defaults
PAL_Flash_StatusTypeDef Metadata_Save(Metadata *m); // erases metadata page & writes new
PAL_Flash_StatusTypeDef Metadata_IncrementBootCount(Metadata *m);
PAL_Flash_StatusTypeDef Metadata_UpdateAfterRecieve(Metadata *m, bool SLOTA_LATEST);

#endif