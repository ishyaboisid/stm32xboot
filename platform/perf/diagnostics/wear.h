#ifndef WEAR_H
#define WEAR_H

#include <stdint.h>

extern volatile uint32_t g_flash_page_erase_count;

void BootPlatform_IncrementFlashEraseCount(void);
uint32_t BootPlatform_GetFlashEraseCount(void);

#endif