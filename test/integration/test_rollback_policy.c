#include <string.h>

#include "unity.h"
#include "rollback_policy.h"

void setUp(void) {}
void tearDown(void) {}

static Metadata baseline_metadata(void)
{
    Metadata meta;
    memset(&meta, 0, sizeof meta);
    meta.SLOTA_LATEST = 1U;
    meta.bootcount = 0U;
    meta.runtime_fault_count = 0U;
    meta.image_state = IMG_STATE_TRIAL;
    return meta;
}

void test_boot_attempt_counter_exhaustion_reverts_to_previous_slot(void)
{
    Metadata meta = baseline_metadata();
    meta.bootcount = BOOT_COUNT_MAX + 1U;

    TEST_ASSERT_EQUAL_INT(1, Rollback_Check(&meta));
    TEST_ASSERT_EQUAL_UINT32(0U, meta.SLOTA_LATEST);
    TEST_ASSERT_EQUAL_UINT32(0U, meta.bootcount);
    TEST_ASSERT_EQUAL_UINT32(IMG_STATE_REVERTED, meta.image_state);
}

void test_runtime_fault_counter_exhaustion_reverts_to_previous_slot(void)
{
    Metadata meta = baseline_metadata();
    meta.runtime_fault_count = RUNTIME_BOOT_COUNT_MAX + 1U;

    TEST_ASSERT_EQUAL_INT(1, Rollback_Check(&meta));
    TEST_ASSERT_EQUAL_UINT32(0U, meta.SLOTA_LATEST);
    TEST_ASSERT_EQUAL_UINT32(IMG_STATE_REVERTED, meta.image_state);
}

void test_counters_within_limits_do_not_trigger_rollback(void)
{
    Metadata meta = baseline_metadata();
    meta.bootcount = BOOT_COUNT_MAX;
    meta.runtime_fault_count = RUNTIME_BOOT_COUNT_MAX;

    TEST_ASSERT_EQUAL_INT(0, Rollback_Check(&meta));
    TEST_ASSERT_EQUAL_UINT32(1U, meta.SLOTA_LATEST);
    TEST_ASSERT_EQUAL_UINT32(IMG_STATE_TRIAL, meta.image_state);
}
