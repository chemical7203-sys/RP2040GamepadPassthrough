cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(rp2040_web_gamepad_firmware C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# --- Executable and Sources ---

# 1. First, define the executable with its main source file
add_executable(${PROJECT_NAME}
    main.c
)

# 2. Add lwIP options, specifying the target
pico_add_lwipopts_files(${PROJECT_NAME}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lwipopts.h
)

# 3. Generate the web UI filesystem from the httpd directory
pico_generate_lwip_fs(${PROJECT_NAME}_fsdata ${CMAKE_CURRENT_SOURCE_DIR}/httpd)

# 4. Add the generated filesystem as a source for our executable
target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_fsdata})


# --- Libraries and Compile Definitions ---

# 5. Link common libraries required by both board versions
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_uart
    pico_tinyusb
)

# 6. Conditionally link board-specific networking libraries and set up configs
if (PICO_BOARD STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_cyw43_arch_lwip_sys_freertos)
    add_definitions(-DPICO_BOARD_PICO_W=1)
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico_w.h)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_lwip_freertos)
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico.h)
endif()

# 7. Pass the selected TinyUSB config file path to the compiler
add_compile_definitions(TUSB_CONFIG_FILE="${TUSB_CONFIG_FILE}")


# --- Final Output ---

# 8. Add support for creating the .uf2 file
pico_add_extra_outputs(${PROJECT_NAME})
