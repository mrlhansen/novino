#pragma once

#include <kernel/usb/usb-core.h>

// Hub Descriptor (0x29)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t  bPwrOn2PwrGood;
    uint8_t  bHubContrCurrent;
    uint16_t DeviceRemovable;
} __attribute__((packed)) usb_hub_descriptor_t;

// Enhanced SuperSpeed Hub Descriptor (0x2A)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t  bPwrOn2PwrGood;
    uint8_t  bHubContrCurrent;
    uint8_t  bHubHdrDecLat;
    uint16_t wHubDelay;
    uint16_t DeviceRemovable;
} __attribute__((packed)) usb3_hub_descriptor_t;

// USB 2.0 Specification: Table 11-17: Hub Class Feature Selectors
enum {
    HUB_FEATURE_PORT_CONNECTION      = 0,
    HUB_FEATURE_PORT_ENABLE          = 1,
    HUB_FEATURE_PORT_SUSPEND         = 2,
    HUB_FEATURE_PORT_OVER_CURRENT    = 3,
    HUB_FEATURE_PORT_RESET           = 4,
    HUB_FEATURE_PORT_POWER           = 8,
    HUB_FEATURE_PORT_LOW_SPEED       = 9,
    HUB_FEATURE_C_PORT_CONNECTION    = 16,
    HUB_FEATURE_C_PORT_ENABLE        = 17,
    HUB_FEATURE_C_PORT_SUSPEND       = 18,
    HUB_FEATURE_C_PORT_OVER_CURRENT  = 19,
    HUB_FEATURE_C_PORT_RESET         = 20,
    HUB_FEATURE_PORT_TEST            = 21,
    HUB_FEATURE_PORT_INDICATOR       = 22,
};

// USB 2.0 Specification: Table 11-21: Port Status Field
enum {
    HUB_PORT_STATUS_CONNECTION    = (1 << 0),
    HUB_PORT_STATUS_ENABLE        = (1 << 1),
    HUB_PORT_STATUS_SUSPEND       = (1 << 2),
    HUB_PORT_STATUS_OVER_CURRENT  = (1 << 3),
    HUB_PORT_STATUS_RESET         = (1 << 4),
    HUB_PORT_STATUS_POWER         = (1 << 8),
    HUB_PORT_STATUS_LOW_SPEED     = (1 << 9),
    HUB_PORT_STATUS_HIGH_SPEED    = (1 << 10),
    HUB_PORT_STATUS_TEST          = (1 << 11),
    HUB_PORT_STATUS_INDICATOR     = (1 << 12),
    HUB_PORT_CHANGE_CONNECTION    = (1 << 16),
    HUB_PORT_CHANGE_ENABLE        = (1 << 17),
    HUB_PORT_CHANGE_SUSPEND       = (1 << 18),
    HUB_PORT_CHANGE_OVER_CURRENT  = (1 << 19),
    HUB_PORT_CHANGE_RESET         = (1 << 20),
};

// Hub Descriptor Types
enum {
    USB_DESCRIPTOR_HUB     = 0x29,
    USB_DESCRIPTOR_SS_HUB  = 0x2A,
};

// bRequest for Hub
enum {
    USB_B_REQUEST_HUB_CLEAR_TT_BUFFER    = 8,
    USB_B_REQUEST_HUB_RESET_TT           = 9,
    USB_B_REQUEST_HUB_GET_TT_STATE       = 10,
    USB_B_REQUEST_HUB_CSTOP_TT           = 11,
    USB_B_REQUEST_HUB_SET_DEPTH          = 12,
    USB_B_REQUEST_HUB_GET_PORT_ERR_COUNT = 13,
};

void usb_hub_init(usb_dev_t*);
