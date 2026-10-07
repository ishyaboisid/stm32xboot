# STM32H743ZI Nucleo board configuration
# populate when porting to H7

set(BOARD_DEFINES
    STM32H743xx
    USE_HAL_DRIVER
    BOARD_H743ZI
)

set(BOARD_CPU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
)

# set(BOARD_LINKER_SCRIPT ...)
# set(BOARD_SOURCES ...)
# set(BOARD_INCLUDES ...)
# set(HAL_SOURCES ...)

message(FATAL_ERROR "H743ZI board not yet implemented")