#include "pal/pal_dwt.h"
#include "stm32f1xx_hal.h"

void PAL_DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // power up the DWT block
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}
uint32_t PAL_DWT_Cycles(void) { return DWT->CYCCNT; }
uint32_t PAL_DWT_CoreHz(void) { return HAL_RCC_GetHCLKFreq(); }
