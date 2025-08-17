#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

//--------------------------------------------------------------------
// BOARD CONFIGURATION
//--------------------------------------------------------------------
#define BOARD_DEVICE_RHPORT_NUM     0
#define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_FULL_SPEED
#define CFG_TUSB_OS                 OPT_OS_FREERTOS

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | BOARD_DEVICE_RHPORT_SPEED)
#define CFG_TUD_ENDPOINT0_SIZE      64

//--------------------------------------------------------------------
// CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_HID              1  // HID is always enabled

#ifdef PICO_BOARD_PICO_W
  // Pico W uses CDC for debugging alongside Wi-Fi for web UI
  #define CFG_TUD_CDC            1
  #define CFG_TUD_RNDIS          0
#else
  // Standard Pico uses RNDIS for web UI over USB
  #define CFG_TUD_CDC            0
  #define CFG_TUD_RNDIS          1
#endif


//--------------------------------------------------------------------
// HID, CDC, RNDIS CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_HID_EP_BUFSIZE      64
#define CFG_TUD_CDC_RX_BUFSIZE      64
#define CFG_TUD_CDC_TX_BUFSIZE      64
#define CFG_TUD_NET_MAC_ADDRESS     {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00}


//--------------------------------------------------------------------
// USB DESCRIPTOR CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_VID                 0x2E8A // Raspberry Pi VID
#define CFG_TUD_PID                 0x1A0B // Custom PID for this project
#define CFG_TUD_MAFU_STRING         "Jules for Sweet"
#define CFG_TUD_PRODUCT_STRING      "RP2040 Advanced Gamepad"
#define CFG_TUD_SERIAL_STRING       "1234567890"

#endif /* _TUSB_CONFIG_H_ */
