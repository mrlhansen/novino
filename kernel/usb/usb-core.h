#pragma once

#include <kernel/usb/usbspecs.h>
#include <kernel/sched/kthreads.h>
#include <kernel/sched/spinlock.h>

enum {
    USB_PORT_ENABLE               = (1 << 0),
    USB_PORT_RESET                = (1 << 1),
    USB_PORT_POWER                = (1 << 2),

    USB_PORT_CHANGE_CONNECTION    = (1 << 3),
    USB_PORT_CHANGE_ENABLE        = (1 << 4),
    USB_PORT_CHANGE_OVER_CURRENT  = (1 << 5),
    USB_PORT_CHANGE_RESET         = (1 << 6),

    USB_PORT_STATUS_CONNECTION    = (1 << 7),
    USB_PORT_STATUS_ENABLE        = (1 << 8),
    USB_PORT_STATUS_OVER_CURRENT  = (1 << 9),
    USB_PORT_STATUS_RESET         = (1 << 10),
    USB_PORT_STATUS_POWER         = (1 << 11),
};

typedef struct __usb_hub usb_hub_t;
typedef struct __usb_dev usb_dev_t;

struct __usb_hub {
    int depth;            // Hub depth
    int speed;            // Hub speed
    int port_id;          // Upstream Port ID
    int slot_id;          // Hub Slot ID
    int number_of_ports;  // Number of downstream ports

    int bus_id;           // Internal Bus ID
    usb_hub_t *parent;    // Parent
    usb_hub_t *next;      // Siblings
    usb_hub_t *child;     // Children
    usb_dev_t *port;      // Devices

    usb_dev_t *dev;       // Device for non-root hubs
    void *hci;            // Pointer to host controller data
    int dci;              // Index for status change endpoint

    struct {
        thread_t *thread;
        volatile uint32_t status;
        volatile uint32_t detach;
        spinlock_t lock;
    } wrk;

    void (*hub_init)(usb_hub_t*);
    void (*hub_fini)(usb_hub_t*);
    void (*hub_wait)(usb_hub_t*);
    void (*dev_init)(usb_dev_t*);
    void (*dev_fini)(usb_dev_t*);
    int  (*port_speed)(usb_hub_t*, int);
    int  (*port_status)(usb_hub_t*, int);
    void (*port_clear_feature)(usb_hub_t*, int, int);
    void (*port_set_feature)(usb_hub_t*, int, int);
};

struct __usb_dev {
    usb_hub_t *parent;
    int present;
    void *data;

    // Topology
    int slot_id;
    int port_id;
    int port_speed;
    int root_port_id;

    // Information
    int bcdUSB;           // BCD encoded USB version
    int bcdDevice;        // BCD encoded device version
    int devClass;         // Device Class
    int devSubClass;      // Device Subclass
    int devProtocol;      // Device Protocol
    int idVendor;         // Device Vendor ID
    int idProduct;        // Device Product ID
    char strVendor[128];  // Device Vendor String
    char strProduct[128]; // Device Product String

    // Descriptors
    uint8_t desc_buf[512];             // Descriptor Buffer
    usb_device_descriptor_t *desc_dev; // Pointer to Device Descriptor
    usb_config_descriptor_t *desc_cfg; // Pointer to Configuration Descriptor

    // Transfer Buffer
    uint64_t buf_phys; // Physical address of transfer buffer
    uint64_t buf_virt; // Virtual address of transfer buffer
    int buf_size;      // Size of transfer buffer
};

void usb_core_port_init(usb_hub_t*, int);
void usb_core_port_fini(usb_hub_t*, int);
void usb_core_port_change(usb_hub_t*, int);

void usb_core_hub_notify(usb_hub_t*, int);
void usb_core_hub_detach(usb_hub_t*);
void usb_core_hub_attach(usb_hub_t*);

void usb_core_print_tree();
