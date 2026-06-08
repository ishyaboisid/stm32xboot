#pragma once

#if defined(BOARD_F103RB)
  #define PAL_MAX_DELAY      0xFFFFFFFFU
  #include "bsp/f103rb/board_config.h"
  #include "bsp/f103rb/pinmap.h"
  #include "stm32/f1/system_init.h"
#elif defined(BOARD_G474RE)
  #include "bsp/g474re/board_config.h"
  #include "bsp/g474re/pinmap.h"
  #include "stm32/g4/system_init.h"
#elif defined(BOARD_H743ZI)
  #include "bsp/h743zi/board_config.h"
  #include "bsp/h743zi/pinmap.h"
  #include "stm32/h7/system_init.h"
#else
  #error "No board selected. Pass -DBOARD_F103RB, -DBOARD_G474RE, or -DBOARD_H743ZI to cmake."
#endif

// abstract PAL interface — same headers regardless of board
// implementations are selected by CMake based on BOARD
#include "pal/pal_uart.h"
#include "pal/pal_flash.h"
#include "pal/pal_gpio.h"
#include "pal/pal_time.h"
#include "pal/pal_crc.h"
#include "pal/pal_iwdg.h"