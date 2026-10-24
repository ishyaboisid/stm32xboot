#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fake_platform.h"
#include "uart_reception.h"

volatile uint16_t uart_size;

void setUp(void)
{
    fake_platform_reset();
}

void tearDown(void) {}

static void queue_u32_le(uint32_t value)
{
    const uint8_t b[4] = {
        (uint8_t)value,
        (uint8_t)(value >> 8),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 24),
    };
    TEST_ASSERT_TRUE(fake_uart_queue(b, sizeof b));
}

static void queue_valid_transfer(const uint8_t *image, size_t image_len)
{
    const uint8_t iv[16] = {0};
    queue_u32_le((uint32_t)image_len);
    TEST_ASSERT_TRUE(fake_uart_queue(iv, sizeof iv));
    TEST_ASSERT_TRUE(fake_uart_queue(image, image_len));
    queue_u32_le(0x12345678U);
}

static Metadata new_metadata(void)
{
    Metadata meta;
    memset(&meta, 0, sizeof meta);
    meta.SLOTA_LATEST = 1U;
    meta.FW_VER_MAJOR = 1U;
    meta.FW_VER_MINOR = 0U;
    meta.BL_VER_MAJOR = 1U;
    meta.BL_VER_MINOR = 0U;
    return meta;
}

static bool tx_contains(const char *needle)
{
    const size_t n = strlen(needle);
    if (n > test_uart.tx_length) return false;
    for (size_t i = 0; i <= test_uart.tx_length - n; ++i) {
        if (memcmp(test_uart.tx_data + i, needle, n) == 0) return true;
    }
    return false;
}

static bool slot_is_erased(const uint8_t *slot, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (slot[i] != 0xFFU) return false;
    }
    return true;
}

static void seed_active_slot(void)
{
    /* Slot A is active when SLOTA_LATEST is 1. */
    fake_slot_a[0] = 0xA5U;
    fake_slot_a[1] = 0x5AU;
}

static void assert_active_slot_preserved(void)
{
    TEST_ASSERT_EQUAL_UINT8(0xA5U, fake_slot_a[0]);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, fake_slot_a[1]);
}

void test_rejects_bad_header_without_flash_write(void)
{
    Metadata meta = new_metadata();
    uint8_t header[4] = {0xFFU, FW_RECEP_START_1, 2U, 0U};

    TEST_ASSERT_EQUAL(RECEP_ERR_START, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    TEST_ASSERT_TRUE(slot_is_erased(fake_slot_b, sizeof fake_slot_b));
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_write_calls);
}

void test_rejects_downgrade_version(void)
{
    Metadata meta = new_metadata();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 0U, 255U};

    TEST_ASSERT_EQUAL(RECEP_ERR_VERSION, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_erase_slot_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_write_calls);
}

void test_rejects_empty_serial_message_without_programming_flash(void)
{
    Metadata meta = new_metadata();
    seed_active_slot();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};

    /* No payload bytes are queued after the header. */
    TEST_ASSERT_EQUAL(RECEP_ERR_RECV, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    TEST_ASSERT_TRUE(slot_is_erased(fake_slot_b, sizeof fake_slot_b));
    assert_active_slot_preserved();
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_write_calls);
}

void test_rejects_empty_image(void)
{
    Metadata meta = new_metadata();
    seed_active_slot();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};
    queue_u32_le(0U);

    TEST_ASSERT_EQUAL(RECEP_ERR_SIZE, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    assert_active_slot_preserved();
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_write_calls);
}

void test_receives_valid_image_into_inactive_slot(void)
{
    static const uint8_t image[] = {0x11U, 0x22U, 0x33U, 0x44U};
    Metadata meta = new_metadata();
    seed_active_slot();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};
    queue_valid_transfer(image, sizeof image);

    TEST_ASSERT_EQUAL(RECEP_OK, UART_Receive(header, &meta, 0));
    TEST_ASSERT_EQUAL_MEMORY(image, fake_slot_b, sizeof image);
    assert_active_slot_preserved();
    TEST_ASSERT_TRUE(tx_contains("B\r\n"));
    TEST_ASSERT_TRUE(tx_contains("ACK\r\n"));
    TEST_ASSERT_EQUAL_UINT32(1U, fake_signature_calls);
}

void test_erases_image_when_signature_is_rejected(void)
{
    static const uint8_t image[] = {0x11U, 0x22U, 0x33U, 0x44U};
    fake_signature_status = SEC_ERR_SIG;
    Metadata meta = new_metadata();
    seed_active_slot();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};
    queue_valid_transfer(image, sizeof image);

    TEST_ASSERT_EQUAL(RECEP_ERR_SEC, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(slot_is_erased(fake_slot_b, sizeof fake_slot_b));
    assert_active_slot_preserved();
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    TEST_ASSERT_EQUAL_UINT32(1U, fake_signature_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, meta.SLOTA_LATEST);
    TEST_ASSERT_EQUAL_UINT8(1U, meta.FW_VER_MAJOR);
    TEST_ASSERT_EQUAL_UINT8(0U, meta.FW_VER_MINOR);
}

void test_rejects_bad_crc_and_erases_untrusted_image(void)
{
    static const uint8_t image[] = {0x11U, 0x22U, 0x33U, 0x44U};
    fake_crc_status = SEC_ERR_CRC;
    Metadata meta = new_metadata();
    seed_active_slot();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};
    queue_valid_transfer(image, sizeof image);

    TEST_ASSERT_EQUAL(RECEP_ERR_SEC, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    TEST_ASSERT_TRUE(slot_is_erased(fake_slot_b, sizeof fake_slot_b));
    assert_active_slot_preserved();
    TEST_ASSERT_EQUAL_UINT32(0U, fake_signature_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, meta.SLOTA_LATEST);
}

void test_rejects_truncated_later_chunk_and_erases_partial_update(void)
{
    static uint8_t first_chunk[UART_RECEP_CHUNK_SIZE];
    static const uint8_t partial_second_chunk[] = {0x11U, 0x22U};
    const uint8_t iv[16] = {0};
    Metadata meta = new_metadata();
    seed_active_slot();
    memset(first_chunk, 0x3CU, sizeof first_chunk);
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};

    queue_u32_le(UART_RECEP_CHUNK_SIZE + 4U);
    TEST_ASSERT_TRUE(fake_uart_queue(iv, sizeof iv));
    TEST_ASSERT_TRUE(fake_uart_queue(first_chunk, sizeof first_chunk));
    TEST_ASSERT_TRUE(fake_uart_queue(partial_second_chunk, sizeof partial_second_chunk));

    TEST_ASSERT_EQUAL(RECEP_ERR_RECV, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    TEST_ASSERT_TRUE(slot_is_erased(fake_slot_b, sizeof fake_slot_b));
    assert_active_slot_preserved();
    TEST_ASSERT_EQUAL_UINT32(1U, fake_flash_write_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, meta.SLOTA_LATEST);
}

void test_rejects_oversized_image_before_flash_programming(void)
{
    Metadata meta = new_metadata();
    seed_active_slot();
    uint8_t header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};
    queue_u32_le((uint32_t)FAKE_FLASH_SLOT_SIZE + 1U);

    TEST_ASSERT_EQUAL(RECEP_ERR_SIZE, UART_Receive(header, &meta, 0));
    TEST_ASSERT_TRUE(tx_contains("NACK\r\n"));
    assert_active_slot_preserved();
    TEST_ASSERT_TRUE(slot_is_erased(fake_slot_b, sizeof fake_slot_b));
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_write_calls);
}

void test_rejects_downgrade_after_newer_firmware_is_verified(void)
{
    static const uint8_t image[] = {0x11U, 0x22U, 0x33U, 0x44U};
    Metadata meta = new_metadata();
    uint8_t newer_header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 2U, 0U};
    queue_valid_transfer(image, sizeof image);

    TEST_ASSERT_EQUAL(RECEP_OK, UART_Receive(newer_header, &meta, 0));
    TEST_ASSERT_EQUAL_UINT8(2U, meta.FW_VER_MAJOR);

    /* The version floor is now 2.0; a 1.255 image must be rejected early. */
    fake_platform_reset();
    uint8_t older_header[4] = {FW_RECEP_START_0, FW_RECEP_START_1, 1U, 255U};
    TEST_ASSERT_EQUAL(RECEP_ERR_VERSION, UART_Receive(older_header, &meta, 0));
    TEST_ASSERT_EQUAL_UINT8(2U, meta.FW_VER_MAJOR);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_erase_slot_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_flash_write_calls);
}