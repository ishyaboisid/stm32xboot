#include "wear.h"

volatile uint32_t g_flash_page_erase_count = 0U;

void BootPlatform_IncrementFlashEraseCount(void) {
    g_flash_page_erase_count++;
}

uint32_t BootPlatform_GetFlashEraseCount(void) {
    return g_flash_page_erase_count;
}