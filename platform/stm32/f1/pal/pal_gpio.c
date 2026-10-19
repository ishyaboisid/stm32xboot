#include "pal/pal_gpio.h"
#include "stm32f1xx_hal.h"

void PAL_GPIO_WritePin(void *GPIOx, uint16_t GPIO_Pin, PAL_GPIO_PinState PinState) {
    HAL_GPIO_WritePin((GPIO_TypeDef*)GPIOx, GPIO_Pin, PinState);
}
uint8_t PAL_GPIO_ReadPin(void *GPIOx, uint16_t GPIO_Pin) {
    return HAL_GPIO_ReadPin((GPIO_TypeDef*)GPIOx, GPIO_Pin) == GPIO_PIN_SET ? 1 : 0;
}