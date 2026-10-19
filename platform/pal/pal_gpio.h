#pragma once
#include <stdint.h>

typedef enum
{
  PAL_GPIO_PIN_RESET = 0u,
  PAL_GPIO_PIN_SET
} PAL_GPIO_PinState;

void PAL_GPIO_WritePin(void *GPIOx, uint16_t GPIO_Pin, PAL_GPIO_PinState PinState);
uint8_t PAL_GPIO_ReadPin(void *GPIOx, uint16_t GPIO_Pin);