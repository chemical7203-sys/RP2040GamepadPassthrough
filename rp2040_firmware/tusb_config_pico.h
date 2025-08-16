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
#define CFG_TUSB_OS                 OPT_OS_FREERTOS // Use FreeRTOS

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//------------- CLASS DRIVER -------------//
#define CFG_TUD_RNDIS            1  // RNDIS (USB Ethernet) 인터페이스 활성화
#define CFG_TUD_HID              1  // HID 인터페이스 활성화

//--------------------------------------------------------------------
// HID CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_HID_EP_BUFSIZE   64

//--------------------------------------------------------------------
// RNDIS CONFIGURATION
//--------------------------------------------------------------------
// Define a MAC address. It must be unique.
#define CFG_TUD_NET_MAC_ADDRESS  {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00}


//--------------------------------------------------------------------
// USB DESCRIPTOR CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_VID              0x2E8A // Raspberry Pi VID
#define CFG_TUD_PID              0x000B // Custom PID for this project
#define CFG_TUD_MAFU_STRING      "Raspberry Pi"
#define CFG_TUD_PRODUCT_STRING   "RP2040 Advanced Gamepad (RNDIS)"
#define CFG_TUD_SERIAL_STRING    "1234567891"

#endif /* _TUSB_CONFIG_H_ */
