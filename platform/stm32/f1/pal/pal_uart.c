#include "pal/pal_uart.h"
#include "stm32/f1/system_init.h"
#include "board_config.h"

uint8_t uart_rx_done = 0; 
uint16_t uart_size = 0;

PAL_UART_StatusTypeDef PAL_UART_Transmit(const uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_UART_Transmit(&huart2, pData, Size, Timeout);
}

PAL_UART_StatusTypeDef PAL_UART_Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_UART_Receive(&huart2, pData, Size, Timeout);
}

PAL_UART_StatusTypeDef PAL_UARTEx_Receive_DMA(uint8_t *pData, uint16_t Size) {
    uart_rx_done = 0; uart_size = 0;
    HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(&huart2, pData, Size);
    if (status != HAL_OK) return status;
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
    uint32_t start = HAL_GetTick();
    while (!uart_rx_done || uart_size != Size) {
        if ((HAL_GetTick() - start) > UART_CHUNK_TIMEOUT_MS) {
            HAL_UART_DMAStop(&huart2);
            return PAL_TIMEOUT;
        }
    }
    HAL_UART_DMAStop(&huart2);
    huart2.RxState = HAL_UART_STATE_READY;
    return PAL_OK;
}

PAL_UART_StatusTypeDef PAL_UART_DMAStop() {
    HAL_UART_DMAStop(&huart2);
    huart2.RxState = HAL_UART_STATE_READY;
    uart_rx_done = 0; uart_size = 0;
    return PAL_OK;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2)
    {
        uart_size = Size;
        uart_rx_done = 1;
    }
}