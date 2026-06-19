message(STATUS "CPPCHECK_EXECUTABLE=${CPPCHECK_EXECUTABLE}")
message(STATUS "CLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE}")
find_program(CPPCHECK_EXECUTABLE   cppcheck   REQUIRED)
find_program(
    CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy
    HINTS
        /opt/homebrew/opt/llvm/bin
        /usr/local/opt/llvm/bin
    REQUIRED
)

set(ANALYSIS_SOURCES
    ${CMAKE_SOURCE_DIR}/boot/main.c
    ${CMAKE_SOURCE_DIR}/bootutil/crypto/aes_ctr.c
    ${CMAKE_SOURCE_DIR}/bootutil/crypto/verify.c
    ${CMAKE_SOURCE_DIR}/bootutil/fwupdate/metadata.c
    ${CMAKE_SOURCE_DIR}/bootutil/fwupdate/uart_reception.c
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/system_init.c 
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/pal/pal_crc.c 
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/pal/pal_flash.c 
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/pal/pal_gpio.c
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/pal/pal_iwdg.c
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/pal/pal_time.c 
    ${CMAKE_SOURCE_DIR}/platform/stm32/f1/pal/pal_uart.c 
)

add_custom_target(cppcheck
    COMMAND ${CPPCHECK_EXECUTABLE}
        --enable=all
        --inconclusive
        --force
        --std=c11
        --suppress=missingInclude
        --suppress=unusedFunction
        --suppressions-list=${CMAKE_SOURCE_DIR}/tests/static/cppcheck_suppressions.txt
        --inline-suppr
        --xml
        --xml-version=2
        ${ANALYSIS_SOURCES}
        2> tests/static/cppcheck.xml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "cppcheck: boot/ bootutil/"
    VERBATIM
)

set(CLANG_TIDY_CHECKS
    "bugprone-*,cert-*,clang-analyzer-*,misc-*,readability-*,\
-readability-magic-numbers,-cert-err33-c,-misc-unused-parameters"
)

add_custom_target(clang-tidy
    COMMAND ${CLANG_TIDY_EXECUTABLE}
        -p ${CMAKE_BINARY_DIR}
        ${ANALYSIS_SOURCES}
        --checks=${CLANG_TIDY_CHECKS}
        --warnings-as-errors=bugprone-*,cert-*
        -export-fixes=tests/static/clang-tidy-fixes.yaml
        --
        -std=c11
        -DBOARD_F103RB
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "clang-tidy: boot/ bootutil/"
    VERBATIM
)

add_custom_target(static-analysis
    DEPENDS cppcheck clang-tidy
    COMMENT "Static analysis complete"
)