#pragma once

#ifdef DEBUG

int uart_putchar(int c);

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)

#else

#define DEBUG_PRINTF(...) ((void)0);

#endif
