#ifdef DEBUG
#warning "DEBUG BUILD"
#define DBG_FORCE_UPDATE 1
#else
#define DBG_FORCE_UPDATE 0
#endif

#include "logging.h"
#include "fwupdate/uart_reception.h"
#include "fwupdate/metadata.h"
#include "platform.h"

#if BL_LOG_LEVEL == BL_LOG_PRINTF
#warning "USING PRINTF"
#include "printf-stdarg.c"
int uart_putchar(int c);
#endif

#define BL_VERSION_MAJOR 0
#define BL_VERSION_MINOR 1

void Task_BL_SetLED(void);
void Task_BL_BlinkLED(void);
static void goto_application(Metadata *meta);
static void check_for_update(Metadata *meta, uint8_t AUTOMATIC_UPDATE);
int Rollback_Check(Metadata *m);
void update_image_state(Metadata *m);