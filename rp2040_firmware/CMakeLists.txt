cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(rp2040_web_gamepad_firmware C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# --- Find Python for makefsdata.py script ---
find_package(Python3 REQUIRED)
set(MAKEFSDATA_SCRIPT ${PICO_SDK_PATH}/src/rp2_common/pico_lwip/tools/makefsdata.py)
file(GLOB_RECURSE FS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/httpd/*)

# Define the command to generate fsdata.c
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fsdata.c
    COMMAND ${Python3_EXECUTABLE} ${MAKEFSDATA_SCRIPT} -h -s -r ${CMAKE_CURRENT_SOURCE_DIR}/httpd
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${FS_FILES}
    COMMENT "Generating fsdata.c from httpd directory"
)
add_custom_target(fsdata_generator DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fsdata.c)

# --- Executable Definition ---
add_executable(${PROJECT_NAME}
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/fsdata.c
)
add_dependencies(${PROJECT_NAME} fsdata_generator)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


# --- Library Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_uart
    pico_tinyusb
)
if (PICO_BOARD STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_cyw43_arch_lwip_sys_freertos)
    add_definitions(-DPICO_BOARD_PICO_W=1)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_lwip_freertos)
endif()


# --- Compile Definitions ---
# Add include paths. Manually adding paths as a brute-force fix for a broken environment.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PICO_SDK_PATH}/lib/tinyusb/src
    ${PICO_SDK_PATH}/lib/FreeRTOS-Kernel/include
    ${PICO_SDK_PATH}/lib/FreeRTOS-Kernel/portable/GCC/ARM_CM0
)
add_compile_definitions(LWIP_HDR_OPTS_H="lwipopts.h")


# --- Final Output ---
pico_add_extra_outputs(${PROJECT_NAME})
