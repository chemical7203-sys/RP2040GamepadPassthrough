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
# Add the current directory to the include path so local headers can be found
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (PICO_BOARD STREQUAL "pico_w")
    add_definitions(-DPICO_BOARD_PICO_W=1)
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico_w.h)
else()
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico.h)
endif()
add_compile_definitions(TUSB_CONFIG_FILE="${TUSB_CONFIG_FILE}")
# Add compile definitions to enable lwIP features directly
add_compile_definitions(
    NO_SYS=0
    LWIP_SOCKET=0
    LWIP_NETCONN=0
    LWIP_HTTPD_CGI=1
    LWIP_HTTPD_SSI=1
    LWIP_HTTPD_SSI_INCLUDE_TAG=0
    LWIP_HTTPD_CUSTOM_FILES=1
    LWIP_HTTPD_DYNAMIC_HEADERS=1
    MEM_LIBC_MALLOC=1
    MEMP_MEM_MALLOC=1
    LWIP_ETHERNET=1
    LWIP_ARP=1
    LWIP_DHCP=1
)


# --- Final Output ---
pico_add_extra_outputs(${PROJECT_NAME})
