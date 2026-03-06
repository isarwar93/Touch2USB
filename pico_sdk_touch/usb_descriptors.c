#include "usb_descriptors.h"
#include "class/hid/hid_device.h"
#include <string.h>

// HID report descriptor for single-touch digitizer / touchscreen.
//
// Report layout (8 bytes on the wire, first byte is Report ID = 0x01):
//   Byte 0: Report ID (0x01)
//   Byte 1: [bit0]=Tip Switch  [bit1]=In Range  [bits2-7]=padding (const 0)
//   Byte 2: X low byte   (0 – 799)
//   Byte 3: X high byte
//   Byte 4: Y low byte   (0 – 479)
//   Byte 5: Y high byte
//   Byte 6: Tip Pressure (0 – 255)
//   Byte 7: Contact Count (0 or 1)
//
// KEY FIX: Logical Maximum for X must be 799 (SCREEN_WIDTH-1) and for Y must
// be 479 (SCREEN_HEIGHT-1) so that the Linux kernel registers the correct
// range in input_absinfo.  When this was 32767 the kernel told libinput/evdev
// the axis range was 0-32767, but main.c only sent 0-799/479, so LVGL scaled
// all touches into the top-left 2 % of the display.
//
// Physical Min/Max + Unit fields are added so libinput can compute pixel
// density and correctly flag the device as INPUT_PROP_DIRECT.
const uint8_t desc_hid_report[] = {
  // ── Application collection: Touch Screen ─────────────────────────────
  0x05, 0x0D,             // Usage Page (Digitizer)
  0x09, 0x04,             // Usage (Touch Screen)
  0xA1, 0x01,             // Collection (Application)
  0x85, 0x01,             //   Report ID (1)

  // ── Logical finger contact ────────────────────────────────────────────
  0x05, 0x0D,             //   Usage Page (Digitizer)
  0x09, 0x22,             //   Usage (Finger)
  0xA1, 0x02,             //   Collection (Logical)

  //  Tip Switch – 1 bit
  0x09, 0x42,             //     Usage (Tip Switch)
  0x15, 0x00,             //     Logical Minimum (0)
  0x25, 0x01,             //     Logical Maximum (1)
  0x75, 0x01,             //     Report Size (1)
  0x95, 0x01,             //     Report Count (1)
  0x81, 0x02,             //     Input (Data, Variable, Absolute)

  //  In Range – 1 bit
  0x09, 0x32,             //     Usage (In Range)
  0x81, 0x02,             //     Input (Data, Variable, Absolute)

  //  Padding – 6 bits (fills out byte 1)
  0x95, 0x06,             //     Report Count (6)
  0x81, 0x03,             //     Input (Constant, Variable, Absolute)

  //  X axis – 16 bits, logical range 0-799, physical range 0-799 (pixels)
  0x05, 0x01,             //     Usage Page (Generic Desktop)
  0x09, 0x30,             //     Usage (X)
  0x75, 0x10,             //     Report Size (16)
  0x95, 0x01,             //     Report Count (1)
  0x55, 0x00,             //     Unit Exponent (0)
  0x65, 0x00,             //     Unit (None)
  0x16, 0x00, 0x00,       //     Logical Minimum (0)
  0x26, 0x1F, 0x03,       //     Logical Maximum (799)   ← was 0xFF,0x7F=32767
  0x36, 0x00, 0x00,       //     Physical Minimum (0)
  0x46, 0x1F, 0x03,       //     Physical Maximum (799)
  0x81, 0x02,             //     Input (Data, Variable, Absolute)

  //  Y axis – 16 bits, logical range 0-479, physical range 0-479 (pixels)
  0x09, 0x31,             //     Usage (Y)
  0x16, 0x00, 0x00,       //     Logical Minimum (0)
  0x26, 0xDF, 0x01,       //     Logical Maximum (479)   ← was inherited 32767
  0x36, 0x00, 0x00,       //     Physical Minimum (0)
  0x46, 0xDF, 0x01,       //     Physical Maximum (479)
  0x81, 0x02,             //     Input (Data, Variable, Absolute)

  //  Tip Pressure – 8 bits (0-255)
  0x05, 0x0D,             //     Usage Page (Digitizer)
  0x09, 0x30,             //     Usage (Tip Pressure)
  0x75, 0x08,             //     Report Size (8)
  0x95, 0x01,             //     Report Count (1)
  0x15, 0x00,             //     Logical Minimum (0)
  0x25, 0xFF,             //     Logical Maximum (255)
  0x81, 0x02,             //     Input (Data, Variable, Absolute)

  0xC0,                   //   End Collection (Finger)

  // ── Contact Count – outside Finger, inside Application ───────────────
  0x05, 0x0D,             //   Usage Page (Digitizer)
  0x09, 0x54,             //   Usage (Contact Count)
  0x75, 0x08,             //   Report Size (8)
  0x95, 0x01,             //   Report Count (1)
  0x15, 0x00,             //   Logical Minimum (0)
  0x25, 0x01,             //   Logical Maximum (1)
  0x81, 0x02,             //   Input (Data, Variable, Absolute)

  0xC0                    // End Collection (Application)
};

// Device descriptor
const tusb_desc_device_t desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = 0x00,
  .bDeviceSubClass = 0x00,
  .bDeviceProtocol = 0x00,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = 0xCafe,
  .idProduct = 0x4004,
  .bcdDevice = 0x0100,
  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,
  .bNumConfigurations = 0x01
};

// Configuration descriptor
const uint8_t desc_configuration[] = {
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
  TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), 0x81, 16, 10),
};

// String descriptors
const char* string_desc_arr [] = {
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "Private",                     // 1: Manufacturer
  "Touchscreen",              // 2: Product
  "123456",                      // 3: Serials, should use chip ID
  "Touch input",                   // 4: HID
};

static uint16_t _desc_str[32];

// TinyUSB callbacks
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
  return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
}

uint8_t const * tud_descriptor_device_cb(void) {
  return (uint8_t const *) &desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
  return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  uint8_t chr_count;

  if ( index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++) {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}