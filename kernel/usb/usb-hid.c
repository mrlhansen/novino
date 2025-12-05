#include <kernel/input/input.h>
#include <kernel/usb/usb.h>
#include <kernel/usb/usb-hid.h>
#include <kernel/usb/xhci.h>
#include <kernel/time/time.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>

static void hid_set_protocol(usb_dev_t *dev, int ifnum, int proto)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_INTERFACE,
        .bRequest      = USB_B_REQUEST_HID_SET_PROTOCOL,
        .wValue        = proto, // 0 = Boot Protocol, 1 = Report Protocol
        .wLength       = 0,
        .wIndex        = ifnum,
    };
    usb_request_submit(dev, &req, 0);
}

static void hid_set_idle(usb_dev_t *dev, int ifnum)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_INTERFACE,
        .bRequest      = USB_B_REQUEST_HID_SET_IDLE,
        .wValue        = 0,
        .wLength       = 0,
        .wIndex        = ifnum,
    };
    usb_request_submit(dev, &req, 0);
}

static void hid_get_report(usb_dev_t *dev, int ifnum)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_INTERFACE,
        .bRequest      = USB_B_REQUEST_HID_GET_REPORT,
        .wValue        = (USB_DESCRIPTOR_HID_REPORT << 8),
        .wLength       = 0xFF,
        .wIndex        = ifnum,
    };
    xhci_transfer_control(dev, &req, dev->buf_phys, req.wLength, TRT_IN_DATA);
}

static void hid_set_report(usb_dev_t *dev, int ifnum, int val)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_INTERFACE,
        .bRequest      = USB_B_REQUEST_HID_SET_REPORT,
        .wValue        = 0x0200,
        .wLength       = 0x01,
        .wIndex        = ifnum,
    };
    usb_request_submit(dev, &req, &val);
}

static void hid_interrupt_handler(void *data)
{
    usb_dev_t *dev = data;
    usb_hid_t *hid = dev->data;
    uint64_t report = *(uint64_t*)hid->report_buf_virt;

    if(hid->type == USB_HID_MOUSE)
    {
        report &= 0xFFFFFF;
        input_mouse_usb_boot_protocol(report, hid->report_last);
    }
    else if(hid->type == USB_HID_KEYBOARD)
    {
        input_kbd_usb_boot_protocol(report, hid->report_last);
    }

    hid->report_last = report;
    xhci_transfer_normal(dev, hid->epnum, hid->report_buf_phys, hid->report_size);
}

void usb_hid_init(usb_dev_t *dev)
{
    usb_interface_descriptor_t *ifd = 0;
    usb_endpoint_descriptor_t *epd = 0;
    int type, rsize;

    // Find boot protocol interface
    while(ifd = find_interface_descriptor(dev, ifd), ifd)
    {
        if(ifd->bInterfaceSubClass == 0x01) // Boot Protocol
        {
            break;
        }
    }

    if(ifd == 0)
    {
        return;
    }

    // Find interrupt IN endpoint
    epd = find_endpoint_descriptor(dev, ifd, USB_ENDPOINT_TYPE_INTERRUPT, USB_ENDPOINT_IN);
    if(epd == 0)
    {
        return;
    }

    // Device type
    if(ifd->bInterfaceProtocol == 1)
    {
        type = USB_HID_KEYBOARD;
        rsize = 8;
    }
    else if(ifd->bInterfaceProtocol == 2)
    {
        type = USB_HID_MOUSE;
        // rsize should be 3 for a mouse in boot protocol mode
        // however, my Logitech G203 mouse does not work with this value
        // setting a larger value for rsize will just result in a short packet
        rsize = 8;
    }
    else
    {
        return;
    }

    // Set configuration
    usb_set_configuration(dev, dev->desc_cfg->bConfigurationValue);

    // Set idle (support is only required for keyboards)
    if(type == USB_HID_KEYBOARD)
    {
        hid_set_idle(dev, ifd->bInterfaceNumber);
    }

    // Set protocol (support is only required for boot protocol devices)
    hid_set_protocol(dev, ifd->bInterfaceNumber, 0);
    timer_sleep(100);

    // Set report
    if(type == USB_HID_KEYBOARD)
    {
        hid_set_report(dev, ifd->bInterfaceNumber, 0x05);
    }

    // Allocate
    usb_hid_t *hid = kzalloc(sizeof(usb_hid_t));
    if(hid == 0)
    {
        return;
    }

    dev->data = hid;
    hid->ifd = ifd;
    hid->epd = epd;
    hid->ifnum = ifd->bInterfaceNumber;
    hid->type = type;

    hid->report_size = rsize;
    hid->report_buf_phys = dev->buf_phys + dev->buf_size - rsize;
    hid->report_buf_virt = dev->buf_virt + dev->buf_size - rsize;

    // Interrupt handler
    hid->epnum = xhci_endpoint_configure(dev, epd);
    xhci_endpoint_set_handler(dev, hid->epnum, hid_interrupt_handler, dev);
    xhci_transfer_normal(dev, hid->epnum, hid->report_buf_phys, hid->report_size);
}
