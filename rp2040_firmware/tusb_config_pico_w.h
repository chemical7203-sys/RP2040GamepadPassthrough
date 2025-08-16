#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

//--------------------------------------------------------------------
// BOARD CONFIGURATION
//--------------------------------------------------------------------

#ifndef BOARD_DEVICE_RHPORT_NUM
#define BOARD_DEVICE_RHPORT_NUM     0
#endif

#ifndef BOARD_DEVICE_RHPORT_SPEED
#define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_FULL_SPEED
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | BOARD_DEVICE_RHPORT_SPEED)

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//------------- CLASS DRIVER -------------//
#define CFG_TUD_CDC              1  // CDC(가상 시리얼) 인터페이스 1개 활성화
#define CFG_TUD_HID              1  // HID 인터페이스 1개 활성화

//--------------------------------------------------------------------
// HID CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_HID_EP_BUFSIZE   64

//--------------------------------------------------------------------
// CDC CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_CDC_RX_BUFSIZE   64
#define CFG_TUD_CDC_TX_BUFSIZE   64

//--------------------------------------------------------------------
// USB DESCRIPTOR CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_VID              0x2E8A // Raspberry Pi VID
#define CFG_TUD_PID              0x000A // Pico SDK's default PID
#define CFG_TUD_MAFU_STRING      "Raspberry Pi"
#define CFG_TUD_PRODUCT_STRING   "RP2040 Advanced Gamepad"
#define CFG_TUD_SERIAL_STRING    "1234567890"

#endif /* _TUSB_CONFIG_H_ */
