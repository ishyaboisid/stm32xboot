#ifndef METADATA_TEST_CONFIG_H
#define METADATA_TEST_CONFIG_H

#include <stdint.h>

// host side test storage
extern uint8_t metadata_test_slot1[];
extern uint8_t metadata_test_slot2[];

#define METADATA_ADDRESS_SLOT1 ((uintptr_t)metadata_test_slot1)
#define METADATA_ADDRESS_SLOT2 ((uintptr_t)metadata_test_slot2)

#endif