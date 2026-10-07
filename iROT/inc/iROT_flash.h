#ifndef IROT_FLASH_H
#define IROT_FLASH_H

#include <stdint.h>
#include "iROT_metadata.h"

#define STMBOOT_ADDRESS 0x08002000
#define STMBOOT_NUM_PAGES 24

int iROT_Flash_InstallSTMBoot(IROT_Metadata *m, uint32_t size);

#endif