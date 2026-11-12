#pragma once
#include <stdint.h>

/* Free-running CPU cycle counter for profiling (DWT->CYCCNT, Cortex-M3/M4/M7). */
void PAL_DWT_Init(void);        // enable the counter and set it to 0
uint32_t PAL_DWT_Cycles(void);  // 32-bit, wraps after 2^32 cycles (67 s at 64 MHz): use unsigned subtraction
uint32_t PAL_DWT_CoreHz(void);  // core clock, for cycles -> time
