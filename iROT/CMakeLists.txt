cmake_minimum_required(VERSION 3.22)

# board must be set before project() so toolchain can use it
set(BOARD "f103rb" CACHE STRING "Target board: f103rb | g474re | h743zi")
message(STATUS "Building for board: ${BOARD}")

# load board config — sets BOARD_DEFINES, BOARD_CPU_FLAGS,
# BOARD_LINKER_SCRIPT, BOARD_SOURCES, BOARD_INCLUDES, HAL_SOURCES
include(cmake/boards/${BOARD}.cmake)

project(iROT C CXX ASM)

add_compile_options(${BOARD_CPU_FLAGS})
add_link_options(${BOARD_CPU_FLAGS})

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── boot and bootutil sources ────────────────────────────────────

set(APP_SOURCES
    src/main.c
    src/iROT_metadata.c
    src/iROT_verify.c
    src/iROT_clock.c
    src/iROT_flash.c
    third_party/sha256/src/sha256.c
    third_party/uECC/uECC.c
    src/hal_handles.c
)

set(APP_INCLUDES
    inc/
    third_party/sha256/inc
    third_party/uECC
    third_party
    keys
    /Users/siddharthmanikant/Desktop/Bachelor_Project/stmboot/platform
)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# ── target ───────────────────────────────────────────────────────
add_executable(iROT
    ${BOARD_SOURCES}
    ${HAL_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(iROT PRIVATE
    ${BOARD_INCLUDES}
    ${APP_INCLUDES}
)

target_compile_definitions(iROT PRIVATE
    ${BOARD_DEFINES}
    uECC_SUPPORTS_secp256r1=1
    uECC_SUPPORTS_secp256k1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp160r1=0
    uECC_OPTIMIZATION_LEVEL=0
    uECC_SUPPORT_COMPRESSED_POINT=0
)

target_compile_options(iROT PRIVATE
    -Wall
    -fdata-sections
    -ffunction-sections
    $<$<CONFIG:Debug>:-Os -g3>
    $<$<CONFIG:Release>:-Os>
)

target_compile_options(iROT PRIVATE
    $<$<CONFIG:Release>:-flto>
)

target_link_options(iROT PRIVATE
    $<$<CONFIG:Release>:-s -flto>
)

target_link_options(iROT PRIVATE
    -T${BOARD_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/Bootloader.map
    --specs=nano.specs
    --specs=nosys.specs
)