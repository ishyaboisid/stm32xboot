#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "verify.h"

TEST_SOURCE_FILE("../../lib/sha256/src/sha256.c")
TEST_SOURCE_FILE("../../lib/uECC/uECC.c")

void setUp(void) {}
void tearDown(void) {}

static const uint8_t signed_firmware[] = {
    0x00U, 0x20U, 0x00U, 0x20U, 0x09U, 0x01U, 0x00U, 0x08U,
    0x10U, 0xB5U, 0x04U, 0x4CU, 0x20U, 0x46U, 0xFFU, 0xF7
};

/* Raw ECDSA P-256 signature: 32-byte r followed by 32-byte s. */
static const uint8_t valid_signature[64] = {
    0x4CU, 0x4EU, 0xFFU, 0x7BU, 0x96U, 0x76U, 0xB8U, 0xABU,
    0x8EU, 0x59U, 0x44U, 0xEDU, 0xC5U, 0x93U, 0xEBU, 0x20U,
    0x58U, 0x3EU, 0xE8U, 0xCFU, 0x87U, 0xB8U, 0x99U, 0x53U,
    0x61U, 0xEDU, 0xE5U, 0x6CU, 0xC3U, 0x09U, 0x21U, 0x6DU,
    0x6BU, 0xF4U, 0xABU, 0xCBU, 0xA1U, 0xD7U, 0xA7U, 0x84U,
    0xC1U, 0x1EU, 0xCEU, 0x30U, 0xE4U, 0xD4U, 0x8CU, 0xB8U,
    0x5AU, 0x7BU, 0x16U, 0x7CU, 0x53U, 0x20U, 0x28U, 0x1FU,
    0xCDU, 0xFFU, 0x39U, 0x5BU, 0x48U, 0xBFU, 0xF2U, 0xF8U
};

void test_real_firmware_verifier_accepts_signed_development_fixture(void)
{
    TEST_ASSERT_EQUAL_INT(1, Verify_Firmware(
        (uint8_t *)signed_firmware, sizeof signed_firmware,
        (uint8_t *)valid_signature));
}

void test_real_firmware_verifier_rejects_unsigned_firmware(void)
{
    uint8_t signature[64] = {0};

    TEST_ASSERT_EQUAL_INT(0, Verify_Firmware(
        (uint8_t *)signed_firmware, sizeof signed_firmware, signature));
}

void test_real_firmware_verifier_rejects_malformed_signature(void)
{
    uint8_t signature[64];
    memset(signature, 0xFF, sizeof signature);

    TEST_ASSERT_EQUAL_INT(0, Verify_Firmware(
        (uint8_t *)signed_firmware, sizeof signed_firmware, signature));
}

void test_real_firmware_verifier_rejects_single_bit_firmware_corruption(void)
{
    uint8_t corrupted_firmware[sizeof signed_firmware];
    memcpy(corrupted_firmware, signed_firmware, sizeof signed_firmware);
    corrupted_firmware[7] ^= 0x01U;

    TEST_ASSERT_EQUAL_INT(0, Verify_Firmware(
        corrupted_firmware, sizeof corrupted_firmware,
        (uint8_t *)valid_signature));
}


void test_real_firmware_verifier_rejects_zero_signature(void)
{
    static uint8_t firmware[] = {
        0x00U, 0x20U, 0x00U, 0x20U, 0x09U, 0x01U, 0x00U, 0x08U,
        0x10U, 0xB5U, 0x04U, 0x4CU, 0x20U, 0x46U, 0xFFU, 0xF7
    };
    uint8_t signature[64] = {0};

    TEST_ASSERT_EQUAL_INT(0, Verify_Firmware(firmware, sizeof firmware, signature));
}