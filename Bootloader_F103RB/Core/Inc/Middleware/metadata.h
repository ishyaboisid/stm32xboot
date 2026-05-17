#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include "Middleware/flash.h"

#define METADATA_ADDRESS 0x0801E000
#define METADATA_MAGIC 0xDEADBEEFU // start of metadata
#define BOOT_COUNT_MAX 3U // rollback feature

typedef struct { // todo, make padding explicit and don't leave it to compiler
    uint32_t magic;
    volatile bool SLOTA_LATEST;
    uint32_t bootcount; // increment every boot, clear by successful firmware
    uint8_t FW_VER_MAJOR, FW_VER_MINOR;
} Metadata;
_Static_assert(sizeof(Metadata) == 16, "Metadata struct size not 16b");

Flash_Status Metadata_Load(Metadata *m); // reads from flash, writes defaults
Flash_Status Metadata_Save(Metadata *m); // erases metadata page & writes new
Flash_Status Metadata_IncrementBootCount(Metadata *m);
Flash_Status Metadata_UpdateAfterRecieve(Metadata *m, bool SLOTA_LATEST);

#endif