#!/usr/bin/env bash
# set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="${ROOT}/test/fuzz/build/fuzz_uart_reception"
CORPUS="${ROOT}/test/fuzz/corpus/uart"

CC="${CC:-/opt/homebrew/opt/llvm/bin/clang}"
SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
LLD="$(brew --prefix lld)/bin/ld.lld"

export SDKROOT
mkdir -p "$(dirname "$OUT")"

if [[ ! -x "$CC" ]]; then
    echo "ERROR: Clang not found: $CC" >&2
    exit 1
fi

if [[ ! -x "$LLD" ]]; then
    echo "ERROR: LLVM linker not found: $LLD" >&2
    echo "Install it with: brew install lld" >&2
    exit 1
fi

echo "Compiler: $("$CC" --version | head -n 1)"
echo "macOS SDK: $SDKROOT"
echo "Building fuzz target..."

"$CC" \
  --no-default-config \
  -fuse-ld=lld \
  -isysroot "$SDKROOT" \
  -std=c11 -g -O1 -fno-omit-frame-pointer \
  -fsanitize=fuzzer,address,undefined \
  -DUNIT_TEST -DBOARD_F103RB -DSTM32F103xB -DTEST \
  -I"${ROOT}/stmboot/bootutil" \
  -I"${ROOT}/stmboot/bootutil/meta" \
  -I"${ROOT}/stmboot/bootutil/transport/uart" \
  -I"${ROOT}/platform" \
  -I"${ROOT}/platform/pal" \
  -I"${ROOT}/platform/bsp/f103rb" \
  -I"${ROOT}/test/support" \
  -I"${ROOT}/test/integration/fakes" \
  "${ROOT}/test/fuzz/fuzz_uart_reception.c" \
  "${ROOT}/test/integration/fakes/fake_platform.c" \
  "${ROOT}/stmboot/bootutil/transport/uart/uart_reception.c" \
  -o "$OUT"

echo "Build successful. Starting fuzz test..."

TMP_CORPUS="$(mktemp -d)"
trap 'rm -rf "$TMP_CORPUS"' EXIT

cp -R "${CORPUS}/." "$TMP_CORPUS/"
"$OUT" "$TMP_CORPUS" -max_len=65536 "$@"