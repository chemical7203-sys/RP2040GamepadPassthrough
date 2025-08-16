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
    int16_t  lx, ly, rx, ry; // Corrected: Switch descriptor uses 16-bit sticks
} hid_switch_report_t;

// DS4 report structure
typedef struct __attribute__((packed)) {
    uint8_t  report_id;
    uint8_t  lx, ly, rx, ry;
    uint8_t  hat;
    uint16_t buttons;
    uint8_t  l2, r2;
    // --- Other DS4 specific fields (simplified)
    uint8_t  padding[2];
    int16_t  gyro_x, gyro_y, gyro_z;
    int16_t  accel_x, accel_y, accel_z;
    // ...
} hid_ds4_report_t;


//--------------------------------------------------------------------+
// Descriptors
//--------------------------------------------------------------------+

// HID Report Descriptor for XInput
const uint8_t tud_hid_xinput_report_descriptor[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x10, 0x81, 0x02, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x33, 0x09, 0x34, 0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x04, 0x81, 0x02, 0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0xC0
};

// HID Report Descriptor for Nintendo Switch Pro Controller
const uint8_t tud_hid_switch_report_descriptor[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x10, 0x81, 0x02, 0x05, 0x01, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3B, 0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x81, 0x42, 0x75, 0x04, 0x95, 0x01, 0x81, 0x03, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x25, 0xFF, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02, 0xC0
};

// HID Report Descriptor for DS4
const uint8_t tud_hid_ds4_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (Eng Rot:Angular Pos)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Null State)
    0x65, 0x00,        //   Unit (None)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0E,        //   Usage Maximum (0x0E)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0E,        //   Report Count (14)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x75, 0x06,        //   Report Size (6)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    // ... (This is a simplified version, a full one is much longer)
    0xC0,              // End Collection
};

#endif /* _USB_DESCRIPTORS_H_ */
