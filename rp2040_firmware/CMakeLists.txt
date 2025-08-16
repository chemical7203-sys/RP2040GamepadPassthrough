cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(rp2040_web_gamepad_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# PICO_BOARD는 이제 외부에서 cmake 명령어로 설정됩니다.
# 예: cmake -DPICO_BOARD=pico_w ..

pico_sdk_init()

# lwIP 설정 파일 추가 (pico_generate_lwip_fs보다 먼저 호출 필요)
pico_add_lwipopts_files(
    ${CMAKE_CURRENT_SOURCE_DIR}/lwipopts.h
)

# httpd 디렉토리의 웹 파일들을 C 소스 파일로 변환
pico_generate_lwip_fs(${PROJECT_NAME}_fsdata ${CMAKE_CURRENT_SOURCE_DIR}/httpd)

add_executable(${PROJECT_NAME}
    main.c
    ${${PROJECT_NAME}_fsdata} # 생성된 파일시스템 소스 추가
)

# 공통 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_uart
    pico_tinyusb
)

# Pico W일 경우에만 Wi-Fi 및 네트워킹 관련 라이브러리 추가
if (PICO_BOARD STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_cyw43_arch_lwip_sys_freertos)
    add_definitions(-DPICO_BOARD_PICO_W=1) # C 코드에서 사용할 수 있도록 정의
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico_w.h)
else()
    # 일반 Pico의 RNDIS 네트워킹을 위해 lwIP 라이브러리를 링크합니다.
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_lwip_freertos)
    set(TUSB_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tusb_config_pico.h)
endif()

# 선택된 tusb_config.h 파일을 컴파일러에 전달
add_compile_definitions(TUSB_CONFIG_FILE="${TUSB_CONFIG_FILE}")

# httpd 디렉토리의 웹 파일들을 C 소스 파일로 변환
# pico_generate_lwip_fs( <target> <directory> [ STRIP_PREFIX <prefix> ] )
pico_generate_lwip_fs(${PROJECT_NAME}_fsdata ${CMAKE_CURRENT_SOURCE_DIR}/httpd)


# .uf2 파일 생성을 위한 추가 설정
pico_add_extra_outputs(${PROJECT_NAME})
