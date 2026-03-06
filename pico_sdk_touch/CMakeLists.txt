cmake_minimum_required(VERSION 3.13)

# Locate the Pico SDK.  Set PICO_SDK_PATH in your environment or pass it on
# the cmake command line:  cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH is not set. "
            "Export the variable or pass -DPICO_SDK_PATH=<path> to cmake.")
endif()

# SDK bootstrap — must come before project()
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_touch C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_touch main.c usb_descriptors.c)

# Route stdio over both USB CDC and UART0 (GPIO 0/1)
pico_enable_stdio_usb(pico_touch 1)
pico_enable_stdio_uart(pico_touch 1)

target_include_directories(pico_touch PRIVATE ${PICO_SDK_PATH}/lib/tinyusb/src)

# TinyUSB must be configured as a full-speed device on port 0
target_compile_definitions(pico_touch PRIVATE
    CFG_TUSB_RHPORT0_MODE=OPT_MODE_DEVICE
    CFG_TUD_HID=1
)

target_link_libraries(pico_touch
    pico_stdlib
    hardware_spi
    hardware_pwm
    tinyusb_device
    tinyusb_board
)

# Generate .uf2, .bin, .dis, and .map alongside the ELF
pico_add_extra_outputs(pico_touch)