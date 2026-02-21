cmake_minimum_required(VERSION 3.13)

# Set PICO_SDK_PATH
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH not set")
endif()

# Pull in SDK (must be before project)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_touch C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_definitions(CFG_TUSB_RHPORT0_MODE=OPT_MODE_DEVICE CFG_TUD_HID=1)

add_executable(pico_touch main.c usb_descriptors.c)

pico_enable_stdio_usb(pico_touch 1)
pico_enable_stdio_uart(pico_touch 1)

target_include_directories(pico_touch PRIVATE ${PICO_SDK_PATH}/lib/tinyusb/src)

target_compile_definitions(pico_touch PRIVATE CFG_TUSB_RHPORT0_MODE=OPT_MODE_DEVICE CFG_TUD_HID=1)

# Add TinyUSB for device
target_link_libraries(pico_touch pico_stdlib hardware_spi hardware_pwm tinyusb_device tinyusb_board)

pico_add_extra_outputs(pico_touch)