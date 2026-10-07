// purely for compilation, as pulling in PAL from stmboot
#include "stm32f1xx_hal.h"

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
// CRC_HandleTypeDef hcrc;