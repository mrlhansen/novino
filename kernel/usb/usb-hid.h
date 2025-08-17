#pragma once

#include <kernel/usb/usb-core.h>

// HID Descriptor Types
enum {
    USB_DESCRIPTOR_HID          = 33,
    USB_DESCRIPTOR_HID_REPORT   = 34,
    USB_DESCRIPTOR_HID_PHYSICAL = 35,
};

// bRequest for HID
enum {
    USB_B_REQUEST_HID_GET_REPORT   = 1,
    USB_B_REQUEST_HID_GET_IDLE     = 2,
    USB_B_REQUEST_HID_GET_PROTOCOL = 3,
    USB_B_REQUEST_HID_SET_REPORT   = 9,
    USB_B_REQUEST_HID_SET_IDLE     = 10,
    USB_B_REQUEST_HID_SET_PROTOCOL = 11,
};

// HID Descriptor (0x21)
typedef struct {
    uint8_t  bDescriptorType;
    uint16_t wDescriptorLength;
} __attribute__((packed)) __hid_t;

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    __hid_t  hidDescriptor[];
} __attribute__((packed)) usb_hid_descriptor_t;

// HID Types
enum {
    USB_HID_KEYBOARD = 1,
    USB_HID_MOUSE    = 2,
};

// HID Data
typedef struct {
    usb_interface_descriptor_t *ifd;
    usb_endpoint_descriptor_t *epd;
    int ifnum;
    int epnum;
    int type;

    uint32_t report_size;
    uint64_t report_buf_phys;
    uint64_t report_buf_virt;
    uint64_t report_last;
} usb_hid_t;

void usb_hid_init(usb_dev_t*);
