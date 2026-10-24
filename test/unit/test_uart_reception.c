// tests/unit/test/test_uart_reception.c
#include "unity.h"
#include "uart_reception.h"
#include "mock_img_security.h"

#include "mock_pal_uart.h"
#include "mock_pal_flash.h"
#include "mock_pal_crc.h"
#include "mock_img_security.h"
#include "mock_verify.h"
#include "mock_aes.h"
#include "mock_metadata.h"

uint8_t AES_KEY[16U];

uint32_t uart_size = 0;

void setUp(void) {}
void tearDown(void) {}

void test_UART_Receive_should_reject_wrong_start_byte_0(void) {
    Metadata meta = { .SLOTA_LATEST = 1 };
    uint8_t bad_header[4] = { 0xFF, 0xBB, 1, 3 };

    PAL_UART_Transmit_ExpectAndReturn((uint8_t *)"NACK\r\n", 6, 100, PAL_OK);

    RECEP_STATUS result = UART_Receive(bad_header, &meta, 0);

    TEST_ASSERT_EQUAL(RECEP_ERR_START, result);
}

void test_UART_Receive_should_reject_downgrade_version(void) {
    Metadata meta = {
        .SLOTA_LATEST  = 1,
        .FW_VER_MAJOR  = 2,
        .FW_VER_MINOR  = 0,
    };
    uint8_t header[4] = { 0xAA, 0xBB, 1, 9 }; // v1.9 < v2.0

    PAL_UART_Transmit_ExpectAndReturn((uint8_t *)"NACK\r\n", 6, 100, PAL_OK);

    RECEP_STATUS result = UART_Receive(header, &meta, 0);

    TEST_ASSERT_EQUAL(RECEP_ERR_VERSION, result);
}