#ifndef TEST_FAKE_PLATFORM_H
#define TEST_FAKE_PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "board_config.h"
#include "metadata.h"
#include "img_security.h"
#include "pal_uart.h"
#include "pal_flash.h"

#define FAKE_UART_CAPACITY (64U * 1024U)
#define FAKE_FLASH_SLOT_SIZE (SLOT_NUM_PAGES * FLASH_PAGE_SIZE_BL)

typedef struct
{
    uint8_t rx_data[FAKE_UART_CAPACITY];
    size_t rx_length;
    size_t rx_offset;
    uint8_t tx_data[4096];
    size_t tx_length;
} boot_uf;

extern boot_uf test_uart;
extern uint8_t fake_slot_a[FAKE_FLASH_SLOT_SIZE];
extern uint8_t fake_slot_b[FAKE_FLASH_SLOT_SIZE];
extern SEC_Status fake_crc_status;
extern SEC_Status fake_signature_status;
extern SEC_Status fake_begin_status;
extern SEC_Status fake_process_status;
extern uint32_t fake_flash_write_calls;
extern uint32_t fake_flash_erase_slot_calls;
extern uint32_t fake_signature_calls;

void fake_platform_reset(void);
bool fake_uart_queue(const uint8_t *data, size_t len);

#endif
