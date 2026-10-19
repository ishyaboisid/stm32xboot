# STM32F103RB Nucleo board configuration

set(BOARD_DEFINES
    STM32F103xB
    USE_HAL_DRIVER
    BOARD_F103RB
)

set(BOARD_CPU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

set(BOARD_LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/linker/STM32F103XX_IROT.ld
)

set(BOARD_SOURCES
    # ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/system_init.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/system_stm32f1xx.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/stm32f1xx_hal_msp.c
    # ${CMAKE_SOURCE_DIR}/iROT_hal_msp.c
    # ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/interrupts/stm32f1xx_it.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/startup/startup_stm32f103xb.s
    ${CMAKE_SOURCE_DIR}/../platform/stm32/syscalls.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/sysmem.c
    # ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/pal/pal_uart.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/pal/pal_flash.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/pal/pal_gpio.c
    # ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/pal/pal_time.c
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/pal/pal_crc.c
    # ${CMAKE_SOURCE_DIR}/../platform/stm32/f1/pal/pal_iwdg.c
)

set(BOARD_INCLUDES
    ${CMAKE_SOURCE_DIR}/../platform/stm32/f1
    ${CMAKE_SOURCE_DIR}/../platform/bsp/f103rb
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/../drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/../drivers/CMSIS/Device/ST/STM32F1xx/Include
)

set(HAL_SOURCES
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_crc.c
    # ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
    # ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    # ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_iwdg.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    # ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
    ${CMAKE_SOURCE_DIR}/../drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
)