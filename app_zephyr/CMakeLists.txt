cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
     ${CMAKE_CURRENT_SOURCE_DIR}/../stmboot)

set(DTC_OVERLAY_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/../stmboot/zephyr/dts/stmboot.overlay
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app_zephyr)

target_sources(app PRIVATE src/main.c)

# add_custom_command(
#     TARGET app
#     POST_BUILD
#     COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/../upload.sh
#             ${PROJECT_BINARY_DIR}/zephyr/zephyr.bin
#     COMMENT "STMBoot: signing, encrypting and uploading firmware..." // todo make a python tool just for preparing the image
#     VERBATIM
# )