#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Pico SDK, TinyUSB, FreeRTOS, lwIP
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/crc32.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "FreeRTOS.h"
#include "task.h"

// Networking headers - conditional
#ifdef PICO_BOARD_PICO_W
#include "pico/cyw43_arch.h"
#else
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#endif
#include "lwip/apps/httpd.h"

// --- SETTINGS ---
typedef struct {
    uint32_t magic; uint32_t crc32;
    bool invert_lx, invert_ly, invert_rx, invert_ry;
    uint8_t deadzone_l, deadzone_r;
    uint8_t output_mode; // 0: XInput, 1: Switch, 2: DS4
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
    int16_t lx, ly, rx, ry; uint8_t l2, r2; uint16_t buttons;
    int16_t gyro_x, gyro_y, gyro_z; int16_t accel_x, accel_y, accel_z;
} GamepadPayload;
typedef enum { WAIT_FOR_START, WAIT_FOR_LEN, READ_PAYLOAD, WAIT_FOR_CHECKSUM } ParserState;
hid_xinput_report_t xinput_report = {0};
hid_switch_report_t switch_report = {0};
hid_ds4_report_t ds4_report = {0};

// --- Function Prototypes ---
void load_settings(); void save_settings();
void send_xinput_report(const GamepadPayload* p);
void send_switch_report(const GamepadPayload* p);
void send_ds4_report(const GamepadPayload* p);

// --- Core Logic ---
int16_t apply_deadzone(int16_t v, uint8_t dz) { return abs(v) < ((dz * 32767) / 100) ? 0 : v; }
uint8_t dpad_to_hat(uint16_t buttons) {
    bool u=buttons&(1<<4),d=buttons&(1<<5),l=buttons&(1<<6),r=buttons&(1<<7);
    if(u){if(l)return 7;if(r)return 1;return 0;}if(d){if(l)return 5;if(r)return 3;return 2;}
    if(l)return 6;if(r)return 4;return 8; // 8 is neutral for HID hat
}
void send_xinput_report(const GamepadPayload* p) {
    int16_t lx=apply_deadzone(p->lx,settings.deadzone_l), ly=apply_deadzone(p->ly,settings.deadzone_l);
    int16_t rx=apply_deadzone(p->rx,settings.deadzone_r), ry=apply_deadzone(p->ry,settings.deadzone_r);
    xinput_report.buttons=p->buttons;
    xinput_report.lx=settings.invert_lx?-lx:lx; xinput_report.ly=settings.invert_ly?-ly:ly;
    xinput_report.rx=settings.invert_rx?-rx:rx; xinput_report.ry=settings.invert_ry?-ry:ry;
    xinput_report.l2=p->l2; xinput_report.r2=p->r2;
    if(tud_hid_ready()) tud_hid_report(0, &xinput_report, sizeof(xinput_report));
}
void send_switch_report(const GamepadPayload* p) {
    int16_t lx=apply_deadzone(p->lx,settings.deadzone_l), ly=apply_deadzone(p->ly,settings.deadzone_l);
    int16_t rx=apply_deadzone(p->rx,settings.deadzone_r), ry=apply_deadzone(p->ry,settings.deadzone_r);
    switch_report.buttons=p->buttons; switch_report.hat=dpad_to_hat(p->buttons);
    switch_report.lx=settings.invert_lx?-lx:lx; switch_report.ly=settings.invert_ly?-ly:ly;
    switch_report.rx=settings.invert_rx?-rx:rx; switch_report.ry=settings.invert_ry?-ry:ry;
    // Gyro and Accel are not part of the simplified Switch report struct in this implementation
    if(tud_hid_ready()) tud_hid_report(0, &switch_report, sizeof(switch_report));
}
void send_ds4_report(const GamepadPayload* p) {
    ds4_report.report_id = 1;
    int16_t lx=apply_deadzone(p->lx,settings.deadzone_l), ly=apply_deadzone(p->ly,settings.deadzone_l);
    int16_t rx=apply_deadzone(p->rx,settings.deadzone_r), ry=apply_deadzone(p->ry,settings.deadzone_r);
    ds4_report.lx = ((settings.invert_lx?-lx:lx) / 256) + 128;
    ds4_report.ly = ((settings.invert_ly?-ly:ly) / 256) + 128;
    ds4_report.rx = ((settings.invert_rx?-rx:rx) / 256) + 128;
    ds4_report.ry = ((settings.invert_ry?-ry:ry) / 256) + 128;
    ds4_report.hat = dpad_to_hat(p->buttons);
    ds4_report.buttons = p->buttons; // Button mapping might need adjustment for DS4 layout
    ds4_report.l2 = p->l2; ds4_report.r2 = p->r2;
    ds4_report.gyro_x = p->gyro_x; ds4_report.gyro_y = p->gyro_y; ds4_report.gyro_z = p->gyro_z;
    ds4_report.accel_x = p->accel_x; ds4_report.accel_y = p->accel_y; ds4_report.accel_z = p->accel_z;
    if (tud_hid_ready()) tud_hid_report(0, &ds4_report, sizeof(ds4_report));
}
void process_and_send_hid_report(const GamepadPayload* p) {
    if (settings.output_mode == 1) send_switch_report(p);
    else if (settings.output_mode == 2) send_ds4_report(p);
    else send_xinput_report(p);
}
void parse_uart() {
    static ParserState s=WAIT_FOR_START; static uint8_t l=0,i=0,c=0; static uint8_t b[MAX_PAYLOAD_SIZE];
    while(uart_is_readable(UART_ID)){
        uint8_t ch=uart_getc(UART_ID);
        switch(s){
            case WAIT_FOR_START:if(ch==START_BYTE)s=WAIT_FOR_LEN;break;
            case WAIT_FOR_LEN:l=ch;i=0;s=(l>0&&l<=MAX_PAYLOAD_SIZE)?READ_PAYLOAD:WAIT_FOR_START;break;
            case READ_PAYLOAD:b[i++]=ch;if(i==l)s=WAIT_FOR_CHECKSUM;break;
            case WAIT_FOR_CHECKSUM:c=ch;uint8_t cs=0;for(int j=0;j<l;j++)cs+=b[j];
                if(cs==c&&l==sizeof(GamepadPayload))process_and_send_hid_report((GamepadPayload*)b);
                s=WAIT_FOR_START;break;
        }
    }
}
const char* cgi_settings_handler(int i, int n, char *p[], char *v[]) {
    settings.invert_lx=settings.invert_ly=settings.invert_rx=settings.invert_ry=false;
    for(int j=0;j<n;j++){
        if(strcmp(p[j],"output_mode")==0)settings.output_mode=atoi(v[j]);
        else if(strcmp(p[j],"invert_lx")==0)settings.invert_lx=true;
        else if(strcmp(p[j],"invert_ly")==0)settings.invert_ly=true;
        else if(strcmp(p[j],"invert_rx")==0)settings.invert_rx=true;
        else if(strcmp(p[j],"invert_ry")==0)settings.invert_ry=true;
        else if(strcmp(p[j],"deadzone_l")==0)settings.deadzone_l=atoi(v[j]);
        else if(strcmp(p[j],"deadzone_r")==0)settings.deadzone_r=atoi(v[j]);
    }
    save_settings();
    reset_usb_boot(0,0);
    return "/reboot.html";
}
u16_t ssi_handler(int i, char *pc, int len) {
    switch(i){
        case 0:snprintf(pc,len,"%s",settings.output_mode==0?"selected":"");break;
        case 1:snprintf(pc,len,"%s",settings.output_mode==1?"selected":"");break;
        case 2:snprintf(pc,len,"%s",settings.output_mode==2?"selected":"");break;
        case 3:snprintf(pc,len,"%s",settings.invert_lx?"checked":"");break;
        case 4:snprintf(pc,len,"%s",settings.invert_ly?"checked":"");break;
        case 5:snprintf(pc,len,"%s",settings.invert_rx?"checked":"");break;
        case 6:snprintf(pc,len,"%s",settings.invert_ry?"checked":"");break;
        case 7:snprintf(pc,len,"%d",settings.deadzone_l);break;
        case 8:snprintf(pc,len,"%d",settings.deadzone_r);break;
    }
    return strlen(pc);
}
char const* ssi_tags[] = { "om_0","om_1","om_2","inv_lx","inv_ly","inv_rx","inv_ry","dz_l","dz_r" };
tCGI cgi_handlers[] = {{ "/settings.cgi", cgi_settings_handler }};
void hid_task(void *p) {
    uart_init(UART_ID,BAUD_RATE);
    gpio_set_function(0,GPIO_FUNC_UART);gpio_set_function(1,GPIO_FUNC_UART);
    while(true){tud_task();parse_uart();vTaskDelay(pdMS_TO_TICKS(1));}
}
#ifdef PICO_BOARD_PICO_W
void web_server_task(void *p) {
    cyw43_arch_enable_ap_mode("RP2040_Gamepad_Config","password123",CYW43_AUTH_WPA2_AES_PSK);
    httpd_init();
    http_set_ssi_handler(ssi_handler,ssi_tags,LWIP_ARRAYSIZE(ssi_tags));
    http_set_cgi_handlers(cgi_handlers,LWIP_ARRAYSIZE(cgi_handlers));
    printf("Web server started on Wi-Fi AP.\n");
    vTaskDelete(NULL);
}
#else
static struct netif netif_data;
void web_server_task(void *p) {
    struct netif*n=&netif_data;tcpip_init(NULL,NULL);
    ip4_addr_t ip,m,g;IP4_ADDR(&ip,192,168,7,1);IP4_ADDR(&m,255,255,255,0);IP4_ADDR(&g,192,168,7,1);
    netif_add(n,&ip,&m,&g,NULL,netif_init_fn,tcpip_input);
    netif_set_default(n);netif_set_up(n);
    httpd_init();http_set_ssi_handler(ssi_handler,ssi_tags,LWIP_ARRAYSIZE(ssi_tags));
    http_set_cgi_handlers(cgi_handlers,LWIP_ARRAYSIZE(cgi_handlers));
    printf("Web server started on RNDIS.\nConnect to http://192.168.7.1/\n");
    while(true){sys_check_timeouts();vTaskDelay(pdMS_TO_TICKS(100));}
}
#endif
int main() {
    stdio_init_all();load_settings();
#ifdef PICO_BOARD_PICO_W
    if(cyw43_arch_init())return -1;
#endif
    tusb_init();
    xTaskCreate(hid_task,"HID_Task",1024,NULL,2,NULL);
    xTaskCreate(web_server_task,"WEB_Task",2048,NULL,1,NULL);
    vTaskStartScheduler();
    return 0;
}
void load_settings() {
    const settings_t*fs=(const settings_t*)flash_target_contents;
    uint32_t sc=fs->crc32,cc=crc32_calculate((const uint8_t*)fs,sizeof(settings_t)-sizeof(uint32_t));
    if(fs->magic==SETTINGS_MAGIC&&sc==cc){memcpy(&settings,fs,sizeof(settings_t));
    }else{
        settings.magic=SETTINGS_MAGIC;settings.invert_lx=false;settings.invert_ly=true;
        settings.invert_rx=false;settings.invert_ry=true;
        settings.deadzone_l=5;settings.deadzone_r=5;settings.output_mode=0;
    }
}
void save_settings() {
    settings.crc32=crc32_calculate((const uint8_t*)&settings,sizeof(settings_t)-sizeof(uint32_t));
    uint8_t b[FLASH_PAGE_SIZE];memset(b,0,FLASH_PAGE_SIZE);memcpy(b,&settings,sizeof(settings_t));
    uint32_t i=save_and_disable_interrupts();
    flash_range_erase(FLASH_SETTINGS_OFFSET,FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_SETTINGS_OFFSET,b,FLASH_PAGE_SIZE);
    restore_interrupts(i);
}
uint8_t const* tud_hid_descriptor_report_cb(uint8_t i) {
    if(settings.output_mode==1)return tud_hid_switch_report_descriptor;
    if(settings.output_mode==2)return tud_hid_ds4_report_descriptor;
    return tud_hid_xinput_report_descriptor;
}
void tud_hid_set_report_cb(uint8_t i,uint8_t r,hid_report_type_t rt,uint8_t const* b,uint16_t s){}
uint16_t tud_hid_get_report_cb(uint8_t i,uint8_t r,hid_report_type_t rt,uint8_t* b,uint16_t s){return 0;}
