cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(rp2040_web_gamepad_firmware C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# --- Find Python for makefsdata.py script ---
find_package(Python3 REQUIRED)

# --- Filesystem Generation ---
# Glob all files in our web root directory to use as dependencies
file(GLOB_RECURSE FS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/httpd/*)

# Define the command to generate fsdata.c using the python script
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fsdata.c
    COMMAND ${Python3_EXECUTABLE} ${PICO_SDK_PATH}/lib/lwip/src/apps/http/makefsdata/makefsdata.py -h -s -r ${CMAKE_CURRENT_SOURCE_DIR}/httpd
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${FS_FILES}
    COMMENT "Generating fsdata.c from httpd directory"
)
# Create a target to drive the custom command
add_custom_target(fsdata_generator DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fsdata.c)


# --- Executable Definition ---
add_executable(${PROJECT_NAME}
    main.c
    # Include the generated C file as a source
    ${CMAKE_CURRENT_BINARY_DIR}/fsdata.c
)
# Add a dependency to ensure fsdata.c is generated before the main build
add_dependencies(${PROJECT_NAME} fsdata_generator)
# Add the directory containing the generated header to the include path
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


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


# --- Compile Definitions ---
# Add the current directory to the include path so lwipopts.h can be found
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (PICO_BOARD STREQUAL "pico_w")
    add_definitions(-DPICO_BOARD_PICO_W=1)
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico_w.h)
else()
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico.h)
endif()
add_compile_definitions(TUSB_CONFIG_FILE="${TUSB_CONFIG_FILE}")
add_compile_definitions(LWIP_HDR_OPTS_H="lwipopts.h")


# --- Final Output ---
pico_add_extra_outputs(${PROJECT_NAME})
