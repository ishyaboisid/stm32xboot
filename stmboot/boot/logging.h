#pragma once
#include "stm32f1xx_hal.h"

#ifndef BL_LOG_LEVEL
#define BL_LOG_LEVEL BL_LOG_UART
#endif

#define BL_LOG_NONE 0
#define BL_LOG_UART 1
#define BL_LOG_PRINTF 2

#if BL_LOG_LEVEL == BL_LOG_NONE
#define BL_LOG(...) ((void)0);
#elif BL_LOG_LEVEL == BL_LOG_UART
extern UART_HandleTypeDef huart2;
#define BL_LOG(msg) HAL_UART_Transmit(&huart2, (uint8_t *)(msg), sizeof(msg) - 1, 100)
#elif BL_LOG_LEVEL == BL_LOG_PRINTF
int uart_putchar(int c);
#define BL_LOG(...) printf(__VA_ARGS__) // uses printf-stdarg.c for printf
#endif