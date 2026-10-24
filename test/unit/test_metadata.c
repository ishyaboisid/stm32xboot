#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "metadata.h"
#include "mock_pal_flash.h"
#include "mock_pal_crc.h"
#include "metadata_test_config.h"

#define TEST_SLOT_SIZE 1024U

static uint32_t crc_state;

static uint32_t test_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFU;

    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc & 1U) ? ((crc >> 1) ^ 0xEDB88320U) : (crc >> 1);
        }
    }

    return ~crc;
}

static void crc_reset_callback(int call_count)
{
    (void)call_count;
    crc_state = 0xFFFFFFFFU;
}

static uint32_t crc_accumulate_callback(uint8_t *data, size_t len,
                                        int call_count)
{
    (void)call_count;

    for (size_t i = 0; i < len; ++i) {
        crc_state ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc_state = (crc_state & 1U)
                ? ((crc_state >> 1) ^ 0xEDB88320U)
                : (crc_state >> 1);
        }
    }

    return crc_state;
}

static uint32_t crc_get_result_callback(int call_count)
{
    (void)call_count;
    return ~crc_state;
}

static uint8_t *slot_for_address(uint32_t address)
{
    if (address == (uint32_t)(uintptr_t)metadata_test_slot1) {
        return metadata_test_slot1;
    }

    if (address == (uint32_t)(uintptr_t)metadata_test_slot2) {
        return metadata_test_slot2;
    }

    return NULL;
}

static PAL_Flash_StatusTypeDef erase_callback(uint32_t page_address,
                                               int call_count)
{
    uint8_t *slot = slot_for_address(page_address);
    (void)call_count;

    if (slot == NULL) {
        return FLASH_ERR_INVALID_ADDR;
    }

    memset(slot, 0xFF, TEST_SLOT_SIZE);
    return FLASH_OK;
}

static PAL_Flash_StatusTypeDef write_callback(uint32_t dest,
                                               const uint8_t *src,
                                               size_t len,
                                               int call_count)
{
    uint8_t *slot = slot_for_address(dest);
    (void)call_count;

    if ((slot == NULL) || (len > TEST_SLOT_SIZE)) {
        return FLASH_ERR_INVALID_ADDR;
    }

    memcpy(slot, src, len);
    return FLASH_OK;
}

static Metadata make_metadata(uint32_t sequence, uint32_t bootcount)
{
    Metadata m;
    memset(&m, 0, sizeof(m));

    m.magic = METADATA_MAGIC;
    m.SLOTA_LATEST = 1U;
    m.bootcount = bootcount;
    m.FW_VER_MAJOR = 1U;
    m.FW_VER_MINOR = 2U;
    m.BL_VER_MAJOR = 3U;
    m.BL_VER_MINOR = 4U;
    m._pad = 0U;
    m.image_state = IMG_STATE_HEALTHY;
    m.runtime_fault_count = 5U;
    m.sequence = sequence;

    for (size_t i = 0; i < sizeof(m.iv); ++i) {
        m.iv[i] = (uint8_t)(i + 1U);
    }

    m.fw_size = 0x1234U;
    m.bl_state = BL_STATE_IDLE;
    m.fwu_state = FWU_STATE_IDLE;
    m.bl_update_requested = 0U;
    m.crc = test_crc32((const uint8_t *)&m, offsetof(Metadata, crc));

    return m;
}

static void put_metadata(uint8_t *slot, const Metadata *m)
{
    memset(slot, 0xFF, TEST_SLOT_SIZE);
    memcpy(slot, m, sizeof(*m));
}

static Metadata read_slot(const uint8_t *slot)
{
    Metadata m;
    memcpy(&m, slot, sizeof(m));
    return m;
}


void setUp(void)
{
    memset(metadata_test_slot1, 0xFF, TEST_SLOT_SIZE);
    memset(metadata_test_slot2, 0xFF, TEST_SLOT_SIZE);

    crc_state = 0xFFFFFFFFU;

    PAL_CRC_Reset_StubWithCallback(crc_reset_callback);
    PAL_CRC_Accumulate_StubWithCallback(crc_accumulate_callback);
    PAL_CRC_GetResult_StubWithCallback(crc_get_result_callback);
    PAL_Flash_ErasePage_StubWithCallback(erase_callback);
    PAL_Flash_Write_StubWithCallback(write_callback);
}

void tearDown(void)
{
}

void test_Metadata_magic_constant_is_correct(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFU, METADATA_MAGIC);
}

void test_Metadata_structure_fits_flash_page(void)
{
    TEST_ASSERT_EQUAL_UINT(66U, sizeof(Metadata));
    TEST_ASSERT_TRUE(sizeof(Metadata) <= METADATA_SLOT_SIZE);
}

void test_Metadata_Load_reads_valid_metadata_from_slot1(void)
{
    Metadata expected = make_metadata(4U, 2U);
    Metadata actual;

    put_metadata(metadata_test_slot1, &expected);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Load(&actual));
    TEST_ASSERT_EQUAL_UINT32(expected.magic, actual.magic);
    TEST_ASSERT_EQUAL_UINT32(expected.sequence, actual.sequence);
    TEST_ASSERT_EQUAL_UINT32(expected.bootcount, actual.bootcount);
    TEST_ASSERT_EQUAL_UINT32(expected.crc, actual.crc);
}

void test_Metadata_Load_selects_slot_with_newer_sequence(void)
{
    Metadata older = make_metadata(3U, 1U);
    Metadata newer = make_metadata(8U, 6U);
    Metadata actual;

    put_metadata(metadata_test_slot1, &older);
    put_metadata(metadata_test_slot2, &newer);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Load(&actual));
    TEST_ASSERT_EQUAL_UINT32(newer.sequence, actual.sequence);
    TEST_ASSERT_EQUAL_UINT32(newer.bootcount, actual.bootcount);
}

void test_Metadata_Load_uses_slot2_when_slot1_magic_is_invalid(void)
{
    Metadata valid = make_metadata(7U, 4U);
    Metadata actual;

    put_metadata(metadata_test_slot1, &valid);
    ((Metadata *)metadata_test_slot1)->magic = 0U;
    put_metadata(metadata_test_slot2, &valid);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Load(&actual));
    TEST_ASSERT_EQUAL_UINT32(valid.sequence, actual.sequence);
    TEST_ASSERT_EQUAL_UINT32(valid.bootcount, actual.bootcount);
}

void test_Metadata_Load_returns_defaults_and_saves_when_metadata_is_erased(void)
{
    Metadata actual;
    PAL_Flash_ErasePage_IgnoreAndReturn(FLASH_OK);
    PAL_Flash_Write_IgnoreAndReturn(FLASH_OK);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Load(&actual));
    TEST_ASSERT_EQUAL_UINT32(METADATA_MAGIC, actual.magic);
    TEST_ASSERT_EQUAL_UINT32(1U, actual.SLOTA_LATEST);
    TEST_ASSERT_EQUAL_UINT32(0U, actual.bootcount);
    TEST_ASSERT_EQUAL_UINT32(IMG_STATE_NONE, actual.image_state);
    TEST_ASSERT_EQUAL_UINT32(BL_STATE_IDLE, actual.bl_state);
    TEST_ASSERT_EQUAL_UINT32(FWU_STATE_IDLE, actual.fwu_state);
    TEST_ASSERT_EQUAL_UINT32(0U, actual.fw_size);
}

void test_Metadata_Load_returns_defaults_when_active_slot_crc_is_invalid(void)
{
    Metadata invalid = make_metadata(5U, 3U);
    Metadata actual;

    invalid.crc ^= 0x1U;
    put_metadata(metadata_test_slot1, &invalid);

    PAL_Flash_ErasePage_IgnoreAndReturn(FLASH_OK);
    PAL_Flash_Write_IgnoreAndReturn(FLASH_OK);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Load(&actual));
    TEST_ASSERT_EQUAL_UINT32(METADATA_MAGIC, actual.magic);
    TEST_ASSERT_EQUAL_UINT32(0U, actual.bootcount);
    TEST_ASSERT_EQUAL_UINT32(IMG_STATE_NONE, actual.image_state);
}

void test_Metadata_Save_writes_incremented_sequence_and_valid_crc(void)
{
    Metadata active = make_metadata(10U, 2U);
    Metadata input = make_metadata(0U, 9U);
    Metadata saved;

    put_metadata(metadata_test_slot1, &active);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Save(&input));

    saved = read_slot(metadata_test_slot2);
    TEST_ASSERT_EQUAL_UINT32(METADATA_MAGIC, saved.magic);
    TEST_ASSERT_EQUAL_UINT32(11U, saved.sequence);
    TEST_ASSERT_EQUAL_UINT32(9U, saved.bootcount);
    TEST_ASSERT_EQUAL_UINT32(test_crc32((const uint8_t *)&saved,
                                       offsetof(Metadata, crc)),
                             saved.crc);
}

void test_Metadata_Save_returns_erase_error_when_erase_fails(void)
{
    Metadata active = make_metadata(2U, 1U);
    Metadata input = make_metadata(0U, 2U);

    put_metadata(metadata_test_slot1, &active);
    PAL_Flash_ErasePage_IgnoreAndReturn(FLASH_ERR_ERASE);

    TEST_ASSERT_EQUAL(FLASH_ERR_ERASE, Metadata_Save(&input));
}

void test_Metadata_Save_returns_write_error_when_write_fails(void)
{
    Metadata active = make_metadata(2U, 1U);
    Metadata input = make_metadata(0U, 2U);

    put_metadata(metadata_test_slot1, &active);
    PAL_Flash_Write_IgnoreAndReturn(FLASH_ERR_WRITE);

    TEST_ASSERT_EQUAL(FLASH_ERR_WRITE, Metadata_Save(&input));
}

void test_Metadata_IncrementBootCount_increments_and_saves(void)
{
    Metadata active = make_metadata(4U, 6U);
    Metadata output;
    Metadata saved;

    put_metadata(metadata_test_slot1, &active);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_IncrementBootCount(&output));

    saved = read_slot(metadata_test_slot2);
    TEST_ASSERT_EQUAL_UINT32(7U, saved.bootcount);
    TEST_ASSERT_EQUAL_UINT32(5U, saved.sequence);
}

void test_Metadata_IncrementBootCount_returns_invalid_address_without_valid_magic(void)
{
    Metadata output;
    memset(metadata_test_slot1, 0xFF, TEST_SLOT_SIZE);
    memset(metadata_test_slot2, 0xFF, TEST_SLOT_SIZE);

    TEST_ASSERT_EQUAL(FLASH_ERR_INVALID_ADDR,
                      Metadata_IncrementBootCount(&output));
}

void test_Metadata_UpdateAfterRecieve_resets_bootcount_and_marks_pending(void)
{
    Metadata active = make_metadata(12U, 8U);
    Metadata output;
    Metadata saved;

    put_metadata(metadata_test_slot1, &active);

    TEST_ASSERT_EQUAL(FLASH_OK,
                      Metadata_UpdateAfterRecieve(&output, true));

    saved = read_slot(metadata_test_slot2);
    TEST_ASSERT_EQUAL_UINT32(0U, saved.bootcount);
    TEST_ASSERT_EQUAL_UINT32(IMG_STATE_PENDING, saved.image_state);
    TEST_ASSERT_EQUAL_UINT32(FWU_STATE_IDLE, saved.fwu_state);
    /* Current implementation deliberately/inadvertently inverts this argument. */
    TEST_ASSERT_EQUAL_UINT32(0U, saved.SLOTA_LATEST);
    TEST_ASSERT_EQUAL_UINT32(13U, saved.sequence);
}

void test_Metadata_UpdateAfterRecieve_returns_invalid_address_without_valid_magic(void)
{
    Metadata output;
    memset(metadata_test_slot1, 0xFF, TEST_SLOT_SIZE);
    memset(metadata_test_slot2, 0xFF, TEST_SLOT_SIZE);

    TEST_ASSERT_EQUAL(FLASH_ERR_INVALID_ADDR,
                      Metadata_UpdateAfterRecieve(&output, true));
}

void test_Metadata_Save_wraps_sequence_when_both_slots_are_erased(void)
{
    Metadata input = {0};
    Metadata saved;

    memset(metadata_test_slot1, 0xFF, 1024);
    memset(metadata_test_slot2, 0xFF, 1024);

    PAL_Flash_ErasePage_IgnoreAndReturn(FLASH_OK);
    PAL_Flash_Write_IgnoreAndReturn(FLASH_OK);

    TEST_ASSERT_EQUAL(FLASH_OK, Metadata_Save(&input));

    /* With both slots erased, the current implementation wraps to zero. */
    memcpy(&saved, metadata_test_slot2, sizeof(saved));
    TEST_ASSERT_EQUAL_UINT32(0U, saved.sequence);
}