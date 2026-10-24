#!/usr/bin/env bash
# set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT}"
command -v cppcheck >/dev/null || { echo "cppcheck is required" >&2; exit 127; }
command -v clang-tidy >/dev/null || { echo "clang-tidy is required" >&2; exit 127; }
command -v clang-format >/dev/null || { echo "clang-format is required" >&2; exit 127; }

CORE_SOURCES=()

for dir in stmboot platform; do # recursively find srcs and headers in stmboot and platform dirs
    while IFS= read -r -d '' file; do
        CORE_SOURCES+=("$file")
    done < <(
        find "$dir" -type f \( -name '*.c' -o -name '*.h' \) -print0
    )
done

INCLUDES=(
    -Istmboot/boot -Istmboot/bootutil -Istmboot/bootutil/meta
    -Istmboot/bootutil/crypto -Istmboot/bootutil/transport/uart
    -Iplatform -Iplatform/pal -Iplatform/bsp/f103rb
    -Ilib -Ilib/uECC -Ilib/sha256/inc -Ilib/aes/inc
    -Itest/support -Itest/integration/fakes -IiROT/inc -IiROT/keys
)
echo "Running cppcheck..."
cppcheck --enable=warning,performance,portability --error-exitcode=1 --std=c11 \
    --inline-suppr --suppress=missingInclude --suppress=missingIncludeSystem \
    "${INCLUDES[@]}" "${CORE_SOURCES[@]}"
echo "Running clang-tidy on host-testable core modules..."
clang-tidy stmboot/bootutil/rollback_policy.c \
    stmboot/bootutil/transport/uart/uart_reception.c \
    stmboot/bootutil/crypto/verify.c \
    --checks='bugprone-*,clang-analyzer-*' \
    --warnings-as-errors='bugprone-*,clang-analyzer-*' -- \
    -std=c11 -DUNIT_TEST -DSTMBOOT_TEST_KEYS -DBOARD_F103RB -DSTM32F103xB \
    "${INCLUDES[@]}"
echo "Checking clang-format on the new signature-test fixtures..."
clang-format --dry-run --Werror \
    test/integration/test_real_signature.c \
    test/integration/fakes/test_public_key.h \
    test/fuzz/fuzz_uart_reception.c
