#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Pico SDK, TinyUSB, FreeRTOS, lwIP
#include "pico/stdlib.hh"
#include "pico/cyw43_arch.h"
#include "pico/bootrom.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/crc32.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/apps/httpd.h"

// --- SETTINGS ---
typedef struct {
    uint32_t magic; uint32_t crc32;
    bool invert_lx, invert_ly, invert_rx, invert_ry;
    uint8_t deadzone_l, deadzone_r;
    uint8_t output_mode; // 0: XInput, 1: Switch
} settings_t;

settings_t settings;
#define SETTINGS_MAGIC 0xDEADBEEF
#define FLASH_SETTINGS_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_SETTINGS_OFFSET);

// --- UART/HID Declarations ---
#define UART_ID uart0
#define BAUD_RATE 230400
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define START_BYTE 0xAA
#define MAX_PAYLOAD_SIZE 32

typedef struct __attribute__((packed)) {
    int16_t lx, ly, rx, ry;
    uint8_t l2, r2;
    uint16_t buttons;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t accel_x, accel_y, accel_z;
} GamepadPayload;
typedef enum { WAIT_FOR_START, WAIT_FOR_LEN, READ_PAYLOAD, WAIT_FOR_CHECKSUM } ParserState;

hid_xinput_report_t xinput_report = {0};
hid_switch_report_t switch_report = {0};

// --- Function Prototypes ---
void load_settings();
void save_settings();
void send_xinput_report(const GamepadPayload* p);
void send_switch_report(const GamepadPayload* p);

// --- Core Logic ---
int16_t apply_deadzone(int16_t v, uint8_t dz) { return abs(v) < ((dz * 32767) / 100) ? 0 : v; }

uint8_t dpad_to_hat(uint16_t buttons) {
    bool up = buttons & (1 << 4); bool down = buttons & (1 << 5);
    bool left = buttons & (1 << 6); bool right = buttons & (1 << 7);
    if (up) { if (left) return 7; if (right) return 1; return 0; }
    if (down) { if (left) return 5; if (right) return 3; return 2; }
    if (left) return 6; if (right) return 4;
    return 8; // Neutral
}

void send_xinput_report(const GamepadPayload* p) {
    int16_t lx = apply_deadzone(p->lx, settings.deadzone_l), ly = apply_deadzone(p->ly, settings.deadzone_l);
    int16_t rx = apply_deadzone(p->rx, settings.deadzone_r), ry = apply_deadzone(p->ry, settings.deadzone_r);
    xinput_report.buttons = p->buttons;
    xinput_report.lx = settings.invert_lx ? -lx : lx; xinput_report.ly = settings.invert_ly ? -ly : ly;
    xinput_report.rx = settings.invert_rx ? -rx : rx; xinput_report.ry = settings.invert_ry ? -ry : ry;
    xinput_report.l2 = p->l2; xinput_report.r2 = p->r2;
    if (tud_hid_ready()) tud_hid_report(0, &xinput_report, sizeof(xinput_report));
}

void send_switch_report(const GamepadPayload* p) {
    int16_t lx = apply_deadzone(p->lx, settings.deadzone_l), ly = apply_deadzone(p->ly, settings.deadzone_l);
    int16_t rx = apply_deadzone(p->rx, settings.deadzone_r), ry = apply_deadzone(p->ry, settings.deadzone_r);
    switch_report.buttons = p->buttons;
    switch_report.hat = dpad_to_hat(p->buttons);
    switch_report.lx = settings.invert_lx ? -lx : lx; switch_report.ly = settings.invert_ly ? -ly : ly;
    switch_report.rx = settings.invert_rx ? -rx : rx; switch_report.ry = settings.invert_ry ? -ry : ry;
    switch_report.gyro_x = p->gyro_x; switch_report.gyro_y = p->gyro_y; switch_report.gyro_z = p->gyro_z;
    switch_report.accel_x = p->accel_x; switch_report.accel_y = p->accel_y; switch_report.accel_z = p->accel_z;
    if (tud_hid_ready()) tud_hid_report(0, &switch_report, sizeof(switch_report));
}

void process_and_send_hid_report(const GamepadPayload* p) {
    if (settings.output_mode == 1) send_switch_report(p);
    else send_xinput_report(p);
}

void parse_uart() {
    static ParserState state = WAIT_FOR_START;
    static uint8_t payload_len = 0, buffer_idx = 0, received_checksum = 0;
    static uint8_t payload_buffer[MAX_PAYLOAD_SIZE];
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        switch (state) {
            case WAIT_FOR_START: if (ch == START_BYTE) state = WAIT_FOR_LEN; break;
            case WAIT_FOR_LEN: payload_len = ch; buffer_idx = 0; state = (ch > 0 && ch <= MAX_PAYLOAD_SIZE) ? READ_PAYLOAD : WAIT_FOR_START; break;
            case READ_PAYLOAD: payload_buffer[buffer_idx++] = ch; if (buffer_idx == payload_len) state = WAIT_FOR_CHECKSUM; break;
            case WAIT_FOR_CHECKSUM: {
                received_checksum = ch; uint8_t calculated_checksum = 0;
                for (int i = 0; i < payload_len; i++) calculated_checksum += payload_buffer[i];
                if (calculated_checksum == received_checksum && payload_len == sizeof(GamepadPayload)) process_and_send_hid_report((GamepadPayload*)payload_buffer);
                state = WAIT_FOR_START; break;
            }
        }
    }
}

// --- Web Server (CGI/SSI) ---
const char * cgi_settings_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
tCGI cgi_handlers[] = {{ "/settings.cgi", cgi_settings_handler }};

u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {
    switch(iIndex) {
        case 0: snprintf(pcInsert, iInsertLen, "%s", settings.output_mode == 0 ? "selected" : ""); break;
        case 1: snprintf(pcInsert, iInsertLen, "%s", settings.output_mode == 1 ? "selected" : ""); break;
        case 2: snprintf(pcInsert, iInsertLen, "%s", settings.output_mode == 2 ? "selected" : ""); break;
        case 3: snprintf(pcInsert, iInsertLen, "%s", settings.invert_lx ? "checked" : ""); break;
        case 4: snprintf(pcInsert, iInsertLen, "%s", settings.invert_ly ? "checked" : ""); break;
        case 5: snprintf(pcInsert, iInsertLen, "%s", settings.invert_rx ? "checked" : ""); break;
        case 6: snprintf(pcInsert, iInsertLen, "%s", settings.invert_ry ? "checked" : ""); break;
        case 7: snprintf(pcInsert, iInsertLen, "%d", settings.deadzone_l); break;
        case 8: snprintf(pcInsert, iInsertLen, "%d", settings.deadzone_r); break;
        default: break;
    }
    return strlen(pcInsert);
}
char const* ssi_tags[] = { "om_0", "om_1", "om_2", "inv_lx", "inv_ly", "inv_rx", "inv_ry", "dz_l", "dz_r" };

const char * cgi_settings_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    settings.invert_lx = settings.invert_ly = settings.invert_rx = settings.invert_ry = false;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "output_mode") == 0) settings.output_mode = atoi(pcValue[i]);
        else if (strcmp(pcParam[i], "invert_lx") == 0) settings.invert_lx = true;
        else if (strcmp(pcParam[i], "invert_ly") == 0) settings.invert_ly = true;
        else if (strcmp(pcParam[i], "invert_rx") == 0) settings.invert_rx = true;
        else if (strcmp(pcParam[i], "invert_ry") == 0) settings.invert_ry = true;
        else if (strcmp(pcParam[i], "deadzone_l") == 0) settings.deadzone_l = atoi(pcValue[i]);
        else if (strcmp(pcParam[i], "deadzone_r") == 0) settings.deadzone_r = atoi(pcValue[i]);
    }
    save_settings();
    reset_usb_boot(0, 0);
    return "/reboot.html";
}

// --- FreeRTOS Tasks & Main ---
void hid_task(void *p) {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(0, GPIO_FUNC_UART); gpio_set_function(1, GPIO_FUNC_UART);
    while (true) { tud_task(); parse_uart(); vTaskDelay(pdMS_TO_TICKS(1)); }
}

void web_server_task(void *p) {
    cyw43_arch_enable_ap_mode("RP2040_Gamepad_Config", "password123", CYW43_AUTH_WPA2_AES_PSK);
    httpd_init();
    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
    printf("Web server started.\n");
    vTaskDelete(NULL);
}

int main() {
    stdio_init_all();
    load_settings();
    if (cyw43_arch_init()) return -1;
    tusb_init();
    xTaskCreate(hid_task, "HID_Task", 1024, NULL, 2, NULL);
    xTaskCreate(web_server_task, "WEB_Task", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
    return 0;
}

// --- Flash & TinyUSB Callbacks ---
void load_settings() {
    const settings_t *flash_settings = (const settings_t *)flash_target_contents;
    uint32_t stored_crc = flash_settings->crc32;
    uint32_t calculated_crc = crc32_calculate((const uint8_t*)flash_settings, sizeof(settings_t) - sizeof(uint32_t));
    if (flash_settings->magic == SETTINGS_MAGIC && stored_crc == calculated_crc) {
        memcpy(&settings, flash_settings, sizeof(settings_t));
    } else {
        settings.magic = SETTINGS_MAGIC;
        settings.invert_lx = false; settings.invert_ly = true;
        settings.invert_rx = false; settings.invert_ry = true;
        settings.deadzone_l = 5; settings.deadzone_r = 5;
        settings.output_mode = 0;
    }
}

void save_settings() {
    settings.crc32 = crc32_calculate((const uint8_t*)&settings, sizeof(settings_t) - sizeof(uint32_t));
    uint8_t buffer[FLASH_PAGE_SIZE];
    memset(buffer, 0, FLASH_PAGE_SIZE);
    memcpy(buffer, &settings, sizeof(settings_t));
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_SETTINGS_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_SETTINGS_OFFSET, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    if (settings.output_mode == 1) return tud_hid_switch_report_descriptor;
    return tud_hid_xinput_report_descriptor;
}
void tud_hid_set_report_cb(uint8_t i, uint8_t r_id, hid_report_type_t rt, uint8_t const* b, uint16_t s) {}
uint16_t tud_hid_get_report_cb(uint8_t i, uint8_t r_id, hid_report_type_t rt, uint8_t* b, uint16_t s) { return 0; }
