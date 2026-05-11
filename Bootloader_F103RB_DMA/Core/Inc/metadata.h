#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include "flash.h"

#define METADATA_ADDRESS 0x0801E000
#define METADATA_MAGIC 0xDEADBEEFU // start of metadata
#define BOOT_COUNT_MAX 3U // rollback feature

typedef struct {
    uint32_t magic;
    volatile bool SLOTA_LATEST;
    uint32_t bootcount; // increment every boot, clear by successful firmware
} Metadata;

Flash_Status Metadata_Load(Metadata *m); // reads from flash, writes defaults
Flash_Status Metadata_Save(Metadata *m); // erases metadata page & writes new
Flash_Status Metadata_IncrementBootCount(Metadata *m);
Flash_Status Metadata_UpdateAfterRecieve(Metadata *m, bool SLOTA_LATEST);

#endif