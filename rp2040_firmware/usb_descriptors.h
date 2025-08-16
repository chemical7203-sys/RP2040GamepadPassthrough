#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#include "tusb.h"

//--------------------------------------------------------------------+
// Shared Report Structs
//--------------------------------------------------------------------+
typedef struct __attribute__((packed)) {
    uint16_t buttons;
    int16_t  lx, ly, rx, ry;
    uint8_t  l2, r2;
} hid_xinput_report_t;

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t  hat;
    int16_t  lx, ly, rx, ry;
    int16_t  gyro_x, gyro_y, gyro_z;
    int16_t  accel_x, accel_y, accel_z;
} hid_switch_report_t;


//--------------------------------------------------------------------+
// Descriptors
//--------------------------------------------------------------------+

// HID Report Descriptor for XInput
const uint8_t tud_hid_xinput_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x10, 0x81, 0x02, // 16 Buttons
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x33, 0x09, 0x34, 0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x04, 0x81, 0x02, // 4 Stick Axes (LX, LY, RX, RY)
    0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, // 2 Trigger Axes (L2, R2)
    0xC0,              // End Collection
};

// HID Report Descriptor for Nintendo Switch Pro Controller
// Note: This is a simplified version for compatibility
const uint8_t tud_hid_switch_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Gamepad)
    0xA1, 0x01,        // Collection (Application)
    // Buttons (2 bytes)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x10,        //   Usage Maximum (16)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    // D-Pad (1 byte)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (Eng Rot:Angular Pos)
    0x81, 0x42,        //   Input (Data,Var,Abs,Null)
    // Padding
    0x75, 0x04,
    0x95, 0x01,
    0x81, 0x03,
    // Sticks (4x 16-bit)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30, 0x09, 0x31, 0x09, 0x33, 0x09, 0x34, //   X, Y, Rx, Ry
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    // Gyro + Accel (6x 16-bit)
    0x05, 0x01,
    0x09, 0x32, 0x09, 0x35, // Z, Rz
    0x09, 0xC4, 0x09, 0xC5, // Motion X, Motion Y
    0x09, 0xC6, 0x09, 0xC7, // Motion Z, Motion Rz
    0x16, 0x00, 0x80,
    0x26, 0xFF, 0x7F,
    0x75, 0x10,
    0x95, 0x06,
    0x81, 0x02,
    0xC0
};

#endif /* _USB_DESCRIPTORS_H_ */
