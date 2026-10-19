#include "pal/pal_time.h"
#include "stm32f1xx_hal.h"

void PAL_Delay(uint32_t Delay) {  
    HAL_Delay(Delay);
}

uint32_t PAL_GetTick(void) {
    return HAL_GetTick();
}