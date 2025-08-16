cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(rp2040_web_gamepad_firmware C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# --- Executable Definition ---
add_executable(${PROJECT_NAME}
    main.c
)

# --- Library Linking ---
# Link libraries FIRST to make their CMake functions available.
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_uart
    pico_tinyusb
)
# Conditionally link the correct networking stack
if (PICO_BOARD STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_cyw43_arch_lwip_sys_freertos)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_lwip_freertos)
endif()


# --- lwIP and Filesystem Configuration ---
# Now that lwIP libraries are linked, these functions will be available.
pico_add_lwipopts_files(${PROJECT_NAME}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lwipopts.h
)
pico_generate_lwip_fs(${PROJECT_NAME}_fsdata
    ${CMAKE_CURRENT_SOURCE_DIR}/httpd
)
target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_fsdata})


# --- Compile Definitions ---
if (PICO_BOARD STREQUAL "pico_w")
    add_definitions(-DPICO_BOARD_PICO_W=1)
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico_w.h)
else()
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico.h)
endif()
add_compile_definitions(TUSB_CONFIG_FILE="${TUSB_CONFIG_FILE}")


# --- Final Output ---
pico_add_extra_outputs(${PROJECT_NAME})
