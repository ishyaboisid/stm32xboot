#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum
{
  PAL_OK       = 0x00U,
  PAL_ERROR    = 0x01U,
  PAL_BUSY     = 0x02U,
  PAL_TIMEOUT  = 0x03U
} PAL_UART_StatusTypeDef;

PAL_UART_StatusTypeDef PAL_UART_Transmit(const uint8_t *pData, uint16_t Size, uint32_t Timeout);
PAL_UART_StatusTypeDef PAL_UART_Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout);
PAL_UART_StatusTypeDef PAL_UARTEx_Receive_DMA(uint8_t *pData, uint16_t Size);
PAL_UART_StatusTypeDef PAL_UART_DMAStop();