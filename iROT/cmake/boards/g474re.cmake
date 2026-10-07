# STM32G474RE Nucleo board configuration
# populate when porting to G4

set(BOARD_DEFINES
    STM32G474xx
    USE_HAL_DRIVER
    BOARD_G474RE
)

set(BOARD_CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# set(BOARD_LINKER_SCRIPT ...)
# set(BOARD_SOURCES ...)
# set(BOARD_INCLUDES ...)
# set(HAL_SOURCES ...)

message(FATAL_ERROR "G474RE board not yet implemented")