#pragma once

#include <kernel/types.h>

// Speed
enum {
    USB_FULL_SPEED       = 1,
    USB_LOW_SPEED        = 2,
    USB_HIGH_SPEED       = 3,
    USB_SUPER_SPEED      = 4,
    USB_SUPER_SPEED_PLUS = 5,
};

// bDeviceClass
enum {
    USB_CLASS_HID          = 0x03,
    USB_CLASS_MASS_STORAGE = 0x08,
    USB_CLASS_HUB          = 0x09,
};

// bmRequestType
enum {
    USB_BM_REQUEST_DEVICE         = (0 << 0), // Recipient: Device
    USB_BM_REQUEST_INTERFACE      = (1 << 0), // Recipient: Interface
    USB_BM_REQUEST_ENDPOINT       = (2 << 0), // Recipient: Endpoint
    USB_BM_REQUEST_OTHER          = (3 << 0), // Recipient: Other
    USB_BM_REQUEST_STANDARD       = (0 << 5), // Type: Standard
    USB_BM_REQUEST_CLASS          = (1 << 5), // Type: Class
    USB_BM_REQUEST_VENDOR         = (2 << 5), // Type: Vendor
    USB_BM_REQUEST_HOST_TO_DEVICE = (0 << 7), // Data Transfer Direction: Host-to-device
    USB_BM_REQUEST_DEVICE_TO_HOST = (1 << 7), // Data Transfer Direction: Device-to-host
};

// bRequest
enum {
    USB_B_REQUEST_GET_STATUS           = 0,
    USB_B_REQUEST_CLEAR_FEATURE        = 1,
    USB_B_REQUEST_SET_FEATURE          = 3,
    USB_B_REQUEST_SET_ADDRESS          = 5,
    USB_B_REQUEST_GET_DESCRIPTOR       = 6,
    USB_B_REQUEST_SET_DESCRIPTOR       = 7,
    USB_B_REQUEST_GET_CONFIGURATION    = 8,
    USB_B_REQUEST_SET_CONFIGURATION    = 9,
    USB_B_REQUEST_GET_INTERFACE        = 10,
    USB_B_REQUEST_SET_INTERFACE        = 11,
    USB_B_REQUEST_SYNCH_FRAME          = 12,
    USB_B_REQUEST_SET_ENCRYPTION       = 13,
    USB_B_REQUEST_GET_ENCRYPTION       = 14,
    USB_B_REQUEST_SET_HANDSHAKE        = 15,
    USB_B_REQUEST_GET_HANDSHAKE        = 16,
    USB_B_REQUEST_SET_CONNECTION       = 17,
    USB_B_REQUEST_SET_SECURITY_DATA    = 18,
    USB_B_REQUEST_GET_SECURITY_DATA    = 19,
    USB_B_REQUEST_SET_WUSB_DATA        = 20,
    USB_B_REQUEST_LOOPBACK_DATA_WRITE  = 21,
    USB_B_REQUEST_LOOPBACK_DATA_READ   = 22,
    USB_B_REQUEST_SET_INTERFACE_DS     = 23,
    USB_B_REQUEST_GET_FW_STATUS        = 26,
    USB_B_REQUEST_SET_FW_STATUS        = 27,
    USB_B_REQUEST_SET_SEL              = 48,
    USB_B_REQUEST_SET_ISOCH_DELAY      = 49,
};

// Descriptor Types
enum {
    USB_DESCRIPTOR_DEVICE                         = 1,
    USB_DESCRIPTOR_CONFIGURATION                  = 2,
    USB_DESCRIPTOR_STRING                         = 3,
    USB_DESCRIPTOR_INTERFACE                      = 4,
    USB_DESCRIPTOR_ENDPOINT                       = 5,
    USB_DESCRIPTOR_INTERFACE_POWER                = 8,
    USB_DESCRIPTOR_OTG                            = 9,
    USB_DESCRIPTOR_DEBUG                          = 10,
    USB_DESCRIPTOR_INTERFACE_ASSOCIATION          = 11,
    USB_DESCRIPTOR_BOS                            = 15,
    USB_DESCRIPTOR_DEVICE_CAPABILITY              = 16,
    USB_DESCRIPTOR_ENDPOINT_COMPANION             = 48,
    USB_DESCRIPTOR_ISOCHRONOUS_ENDPOINT_COMPANION = 49,
};

// Endpoint Transfer Type
enum {
    USB_ENDPOINT_TYPE_CONTROL     = 0,
    USB_ENDPOINT_TYPE_ISOCHRONOUS = 1,
    USB_ENDPOINT_TYPE_BULK        = 2,
    USB_ENDPOINT_TYPE_INTERRUPT   = 3,
};

// Endpoint Direction
enum {
    USB_ENDPOINT_OUT = 0,
    USB_ENDPOINT_IN  = 1,
};

// Device Descriptor (0x01)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// Configuration Descriptor (0x02)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// String Descriptor (0x03)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bString[];
} __attribute__((packed)) usb_string_descriptor_t;

// Interface Descriptor (0x04)
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

// Endpoint Descriptor (0x05)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// Endpoint Companion Descriptor (0x30)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bMaxBurst;
    uint8_t  bmAttributes;
    uint16_t wBytesPerInterval;
} __attribute__((packed)) usb_endpoint_companion_descriptor_t;

// Request
typedef struct {
    uint8_t bmRequestType; // Data Transfer Direction (bit 7), Type (bit 5-6), Recipient (bit 0-4)
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_request_t;
