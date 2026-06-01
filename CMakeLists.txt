cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Nombre lógico del firmware (portable)
get_filename_component(FW_NAME ${CMAKE_SOURCE_DIR} NAME CACHE STRING "Firmware name")

project(${FW_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${FW_NAME})

add_subdirectory(lib/drivers)
add_subdirectory(lib/utils)
add_subdirectory(src)

target_link_libraries(${FW_NAME} PRIVATE
    hardware_spi
    pico_stdlib
    drivers
    utils
)

pico_enable_stdio_usb(${FW_NAME} 1)
pico_enable_stdio_uart(${FW_NAME} 0)

pico_add_extra_outputs(${FW_NAME})

# --- Flash (Debug Probe via OpenOCD) ---
set(PICO_FLASH_SCRIPT "$ENV{HOME}/pico/tools/flash.sh")

if(EXISTS "${PICO_FLASH_SCRIPT}")
    add_custom_target(flash
        COMMAND ${PICO_FLASH_SCRIPT} $<TARGET_FILE:${FW_NAME}>
        DEPENDS ${FW_NAME}
        USES_TERMINAL
    )
else()
    message(WARNING "flash target disabled: no flash script at ${PICO_FLASH_SCRIPT}")
endif()

