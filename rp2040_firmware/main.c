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
#include "lwip/apps/httpd.h"

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

// --- Embedded Web Files ---
const char* INDEX_PAGE =
"<!DOCTYPE html><html><head><title>RP2040 Gamepad Config</title><style>body{font-family:sans-serif;background-color:#f0f0f0;margin:2em}.container{background-color:white;padding:2em;border-radius:8px;box-shadow:0 4px 8px rgba(0,0,0,.1);max-width:600px;margin:auto}h1{color:#333}.form-group{margin-bottom:1.5em}label{display:block;margin-bottom:.5em;font-weight:700}input[type=number],select{width:100%;padding:.5em;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}input[type=checkbox]{margin-right:.5em}.btn{background-color:#007bff;color:#fff;padding:.7em 1.2em;border:none;border-radius:4px;cursor:pointer;font-size:1em}.btn:hover{background-color:#0056b3}.notice{font-size:.9em;color:#666;margin-top:1em}</style></head><body><div class=container><h1>RP2040 Gamepad Configuration</h1><form action=/settings.cgi method=post><div class=form-group><label for=output_mode>Output Mode</label><select id=output_mode name=output_mode><option value=0 <!--#om_0-->>XInput</option><option value=1 <!--#om_1-->>Nintendo Switch</option><option value=2 <!--#om_2-->>DS4</option></select></div><div class=form-group><label>Joystick Inversion</label><input type=checkbox name=invert_lx <!--#inv_lx-->> Invert Left Stick X-Axis<br><input type=checkbox name=invert_ly <!--#inv_ly-->> Invert Left Stick Y-Axis<br><input type=checkbox name=invert_rx <!--#inv_rx-->> Invert Right Stick X-Axis<br><input type=checkbox name=invert_ry <!--#inv_ry-->> Invert Right Stick Y-Axis</div><div class=form-group><label for=deadzone_l>Left Stick Deadzone (%)</label><input type=number id=deadzone_l name=deadzone_l min=0 max=100 value=<!--#dz_l-->></div><div class=form-group><label for=deadzone_r>Right Stick Deadzone (%)</label><input type=number id=deadzone_r name=deadzone_r min=0 max=100 value=<!--#dz_r-->></div><button type=submit class=btn>Save Settings</button><p class=notice>Settings will be saved and the device will reboot.</p></form></div></body></html>";

const char* REBOOT_PAGE =
"<!DOCTYPE html><html><head><title>Rebooting...</title><meta http-equiv=refresh content=\"5;url=/\"><style>body{font-family:sans-serif;background-color:#f0f0f0;margin:2em;text-align:center}.container{background-color:white;padding:2em;border-radius:8px;box-shadow:0 4px 8px rgba(0,0,0,.1);max-width:600px;margin:auto}h1{color:#333}</style></head><body><div class=container><h1>Settings Saved!</h1><p>The device is rebooting to apply the new settings.</p><p>You will be redirected back to the main page in 5 seconds. Please reconnect if needed.</p></div></body></html>";

// --- Custom Filesystem for lwIP ---
int fs_open_custom(struct fs_file *file, const char *name) {
    if (!strcmp(name, "/index.shtml")) {
        file->data = INDEX_PAGE; file->len = strlen(INDEX_PAGE); file->index = file->len; file->pextension = NULL; file->flags = FS_FILE_FLAGS_SSI; return 1;
    } else if (!strcmp(name, "/reboot.html")) {
        file->data = REBOOT_PAGE; file->len = strlen(REBOOT_PAGE); file->index = file->len; file->pextension = NULL; return 1;
    }
    return 0;
}
void fs_close_custom(struct fs_file *file) {}
int fs_read_custom(struct fs_file *file, char *buffer, int count) { return FS_READ_EOF; }

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
    if(l)return 6;if(r)return 4;return 8;
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
    ds4_report.buttons = p->buttons;
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
