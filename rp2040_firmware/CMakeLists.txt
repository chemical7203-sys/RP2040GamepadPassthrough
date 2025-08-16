cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(rp2040_web_gamepad_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pico W 보드를 타겟으로 설정
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    main.c
    ${${PROJECT_NAME}_fsdata} # 생성된 파일시스템 소스 추가
)

# FreeRTOS와 lwIP를 사용하기 위해 라이브러리 추가
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    hardware_uart
    pico_tinyusb
)

# httpd 디렉토리의 웹 파일들을 C 소스 파일로 변환
# pico_generate_lwip_fs( <target> <directory> [ STRIP_PREFIX <prefix> ] )
pico_generate_lwip_fs(${PROJECT_NAME}_fsdata ${CMAKE_CURRENT_SOURCE_DIR}/httpd)


# .uf2 파일 생성을 위한 추가 설정
pico_add_extra_outputs(${PROJECT_NAME})
