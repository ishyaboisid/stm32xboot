#pragma once
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_iwdg.h"
#include "bsp/f103rb/pinmap.h"
#include "bsp/f103rb/board_config.h"
// #include "usb_device.h" // todo usb

extern UART_HandleTypeDef  huart2;
extern CRC_HandleTypeDef   hcrc;
extern DMA_HandleTypeDef   hdma_usart2_rx;
extern DMA_HandleTypeDef   hdma_usart2_tx;
extern IWDG_HandleTypeDef  hiwdg;

void System_Init(void);

void PAL_NVIC_SystemReset(void);

void Error_Handler(void);