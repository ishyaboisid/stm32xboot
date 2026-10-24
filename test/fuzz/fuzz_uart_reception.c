#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fake_platform.h"
#include "uart_reception.h"

volatile uint16_t uart_size;

/*
 * Fuzz the real UART_Receive protocol state machine with a bounded fake UART
 * and flash backend. Slot A is marked as active; every input must preserve it.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t header[4] = {0U, 0U, 0U, 0U};
    Metadata meta;
    static uint8_t active_slot_before[FAKE_FLASH_SLOT_SIZE];
    const size_t header_len = size < sizeof header ? size : sizeof header;
    size_t queued_len;

    fake_platform_reset();
    uart_size = 0U;
    memset(&meta, 0, sizeof meta);
    meta.SLOTA_LATEST = 1U;
    meta.FW_VER_MAJOR = 1U;
    meta.FW_VER_MINOR = 0U;
    meta.BL_VER_MAJOR = 1U;
    meta.BL_VER_MINOR = 0U;

    memset(fake_slot_a, 0xA5, sizeof fake_slot_a);
    memcpy(active_slot_before, fake_slot_a, sizeof fake_slot_a);
    if (header_len != 0U) {
        memcpy(header, data, header_len);
    }

    queued_len = size > sizeof header ? size - sizeof header : 0U;
    if (queued_len > FAKE_UART_CAPACITY) {
        queued_len = FAKE_UART_CAPACITY;
    }
    if (queued_len != 0U) {
        (void)fake_uart_queue(data + sizeof header, queued_len);
    }

    (void)UART_Receive(header, &meta, 0);

    /* A malformed packet must never corrupt the currently active slot. */
    if (memcmp(fake_slot_a, active_slot_before, sizeof fake_slot_a) != 0) {
        abort();
    }
    return 0;
}
