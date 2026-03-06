#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "tusb.h"

/*
 * HID report descriptor (single-touch digitizer, Report ID 1).
 * Defined in usb_descriptors.c.  Length is sizeof(desc_hid_report).
 */
extern const uint8_t desc_hid_report[];

#endif // USB_DESCRIPTORS_H_