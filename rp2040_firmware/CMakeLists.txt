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

# --- Library Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_uart
    pico_tinyusb
)
if (PICO_BOARD STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_cyw43_arch_lwip_sys_freertos)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_lwip_freertos)
endif()


# --- Compile Definitions and Includes ---
# Add include paths as PUBLIC so linked libraries (like tinyusb) can see them.
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (PICO_BOARD STREQUAL "pico_w")
    add_definitions(-DPICO_BOARD_PICO_W=1)
endif()
add_compile_definitions(LWIP_HDR_OPTS_H="lwipopts.h")


# --- Final Output ---
pico_add_extra_outputs(${PROJECT_NAME})
