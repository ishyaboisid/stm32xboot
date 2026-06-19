cmake_minimum_required(VERSION 3.22)

# board must be set before project() so toolchain can use it
set(BOARD "f103rb" CACHE STRING "Target board: f103rb | g474re | h743zi")
message(STATUS "Building for board: ${BOARD}")

# load board config — sets BOARD_DEFINES, BOARD_CPU_FLAGS,
# BOARD_LINKER_SCRIPT, BOARD_SOURCES, BOARD_INCLUDES, HAL_SOURCES
include(cmake/boards/${BOARD}.cmake)

project(Bootloader C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# apply cpu flags globally — must happen after project()
add_compile_options(${BOARD_CPU_FLAGS})
add_link_options(${BOARD_CPU_FLAGS})

# ── boot and bootutil sources ────────────────────────────────────
set(APP_SOURCES
    boot/main.c
    bootutil/fwupdate/uart_reception.c
    bootutil/fwupdate/metadata.c
    bootutil/crypto/verify.c
    bootutil/crypto/aes_ctr.c
    third_party/printf-stdarg.c
    third_party/aes/src/aes.c
    third_party/sha256/src/sha256.c
    third_party/uECC/uECC.c
)

set(APP_INCLUDES
    boot
    bootutil
    bootutil/crypto
    platform
    platform/pal
    third_party/
    third_party/aes/inc
    third_party/sha256/inc
    third_party/uECC
)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# ── static analysis — host build, no ARM toolchain ───────────────────────
# Only included when STATIC_ANALYSIS=ON, set by the preset
if(STATIC_ANALYSIS)
    message(STATUS "Static analysis mode — no firmware target built")
    include(tests/CMakeLists.txt)
    return()  # skip all firmware targets below this point
endif()

# ── target ───────────────────────────────────────────────────────
add_executable(Bootloader
    ${BOARD_SOURCES}
    ${HAL_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(Bootloader PRIVATE
    ${BOARD_INCLUDES}
    ${APP_INCLUDES}
)

target_compile_definitions(Bootloader PRIVATE
    ${BOARD_DEFINES}
    BL_LOG_LEVEL=1
    uECC_SUPPORTS_secp256r1=1
    uECC_SUPPORTS_secp256k1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp160r1=0
    uECC_OPTIMIZATION_LEVEL=0
    uECC_SUPPORT_COMPRESSED_POINT=0
    AES128=1
    CTR=1
    CBC=0
    ECB=0
)

target_compile_options(Bootloader PRIVATE
    -Wall
    -fdata-sections
    -ffunction-sections
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os>
)

target_compile_options(Bootloader PRIVATE
    $<$<CONFIG:Release>:-flto>
)

target_link_options(Bootloader PRIVATE
    $<$<CONFIG:Release>:-s -flto>
)

target_link_options(Bootloader PRIVATE
    -T${BOARD_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/Bootloader.map
    --specs=nano.specs
    --specs=nosys.specs
)

# ── post-build ─────────────────────────────────────
add_custom_command(TARGET Bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:Bootloader>
        ${CMAKE_BINARY_DIR}/Bootloader.bin
    COMMENT "Generating Bootloader.bin"
)

add_custom_command(TARGET Bootloader POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:Bootloader>
    COMMENT "Binary size"
)

add_custom_command(TARGET Bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        $<TARGET_FILE:Bootloader>
        ${CMAKE_BINARY_DIR}/Bootloader.hex
    COMMENT "Generating Bootloader.hex"
)

message(STATUS "ELF output: ${CMAKE_BINARY_DIR}/Bootloader.elf")