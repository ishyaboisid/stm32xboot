if (CONFIG_BOOTLOADER_STMBOOT)

zephyr_library()
zephyr_library_sources(
    ${CMAKE_CURRENT_LIST_DIR}/lib/stmboot_api.c
    ${CMAKE_CURRENT_LIST_DIR}/../bootutil/meta/metadata.c
    ${CMAKE_CURRENT_LIST_DIR}/pal/zephyr_pal_flash.c
    ${CMAKE_CURRENT_LIST_DIR}/pal/zephyr_pal_crc.c
)

zephyr_include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/../bootutil/crypto
    ${CMAKE_CURRENT_LIST_DIR}/../bootutil/transport/uart
    ${CMAKE_CURRENT_LIST_DIR}/../bootutil/meta 
    ${CMAKE_CURRENT_LIST_DIR}/../../platform
    ${CMAKE_CURRENT_LIST_DIR}/../../platform/pal

)

# list(APPEND DTC_OVERLAY_FILE
#     ${CMAKE_CURRENT_LIST_DIR}/dts/stmboot.overlay
# )

# set(DTC_OVERLAY_FILE ${DTC_OVERLAY_FILE} PARENT_SCOPE)

set(STMBOOT_UPLOAD_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/../../upload.sh
    PARENT_SCOPE
)

message(STATUS "STMBoot module loaded")

endif()