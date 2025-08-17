#include <kernel/usb/usb.h>
#include <kernel/usb/usb-hub.h>
#include <kernel/usb/xhci.h>
#include <kernel/mem/heap.h>
#include <kernel/time/time.h>
#include <kernel/debug.h>
#include <string.h>

//
// Hub requests
//

static void hub_get_descriptor(usb_dev_t *dev, usb_hub_descriptor_t *desc)
{
    void *buf;
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_GET_DESCRIPTOR,
        .wValue        = (USB_DESCRIPTOR_HUB << 8),
        .wLength       = 0xFF,
        .wIndex        = 0,
    };
    buf = usb_request_submit(dev, &req, 0);
    memcpy(desc, buf, sizeof(usb_hub_descriptor_t));
}

static uint32_t hub_get_status(usb_dev_t *dev)
{
    uint32_t *buf;
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_GET_STATUS,
        .wValue        = 0,
        .wLength       = 4,
        .wIndex        = 0,
    };
    buf = usb_request_submit(dev, &req, 0);
    return *buf; // First word is wHubStatus and second word is wHubChange
}

static void hub_set_depth(usb_dev_t *dev, int depth)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_HUB_SET_DEPTH,
        .wValue        = depth,
        .wLength       = 0,
        .wIndex        = 0,
    };
    usb_request_submit(dev, &req, 0);
}

static uint32_t port_get_status(usb_dev_t *dev, int port_id)
{
    uint32_t *buf;
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_OTHER,
        .bRequest      = USB_B_REQUEST_GET_STATUS,
        .wValue        = 0,
        .wLength       = 4,
        .wIndex        = port_id,
    };
    buf = usb_request_submit(dev, &req, 0);
    return *buf; // First word is wPortStatus and second word is wPortChange
}

static void port_set_feature(usb_dev_t *dev, int port_id, int value)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_OTHER,
        .bRequest      = USB_B_REQUEST_SET_FEATURE,
        .wValue        = value,
        .wLength       = 0,
        .wIndex        = port_id,
    };
    usb_request_submit(dev, &req, 0);
}

static void port_clear_feature(usb_dev_t *dev, int port_id, int value)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_CLASS | USB_BM_REQUEST_OTHER,
        .bRequest      = USB_B_REQUEST_CLEAR_FEATURE,
        .wValue        = value,
        .wLength       = 0,
        .wIndex        = port_id,
    };
    usb_request_submit(dev, &req, 0);
}

//
// Hub interface
//

static int hub_port_speed(usb_hub_t *hub, int port_id)
{
    uint32_t portsc;
    portsc = port_get_status(hub->dev, port_id);

    if(portsc & HUB_PORT_STATUS_LOW_SPEED)
    {
        return USB_LOW_SPEED;
    }
    else if(portsc & HUB_PORT_STATUS_HIGH_SPEED)
    {
        return USB_HIGH_SPEED;
    }

    return USB_FULL_SPEED;
}

static int hub_port_status(usb_hub_t *hub, int port_id)
{
    uint32_t portsc, status;
    portsc = port_get_status(hub->dev, port_id);
    status = 0;

    if(portsc & HUB_PORT_STATUS_CONNECTION)
    {
        status |= USB_PORT_STATUS_CONNECTION;
    }
    if(portsc & HUB_PORT_STATUS_ENABLE)
    {
        status |= USB_PORT_STATUS_ENABLE;
    }
    if(portsc & HUB_PORT_STATUS_OVER_CURRENT)
    {
        status |= USB_PORT_STATUS_OVER_CURRENT;
    }
    if(portsc & HUB_PORT_STATUS_RESET)
    {
        status |= USB_PORT_STATUS_RESET;
    }
    if(portsc & HUB_PORT_STATUS_POWER)
    {
        status |= USB_PORT_STATUS_POWER;
    }

    if(portsc & HUB_PORT_CHANGE_CONNECTION)
    {
        status |= USB_PORT_CHANGE_CONNECTION;
    }
    if(portsc & HUB_PORT_CHANGE_ENABLE)
    {
        status |= USB_PORT_CHANGE_ENABLE;
    }
    if(portsc & HUB_PORT_CHANGE_OVER_CURRENT)
    {
        status |= USB_PORT_CHANGE_OVER_CURRENT;
    }
    if(portsc & HUB_PORT_CHANGE_RESET)
    {
        status |= USB_PORT_CHANGE_RESET;
    }

    return status;
}

static void hub_port_clear_feature(usb_hub_t *hub, int port_id, int flag)
{
    switch(flag)
    {
        case USB_PORT_POWER:
            port_clear_feature(hub->dev, port_id, HUB_FEATURE_PORT_POWER);
            break;
        case USB_PORT_ENABLE:
            port_clear_feature(hub->dev, port_id, HUB_FEATURE_PORT_ENABLE);
            break;
        case USB_PORT_CHANGE_CONNECTION:
            port_clear_feature(hub->dev, port_id, HUB_FEATURE_C_PORT_CONNECTION);
            break;
        case USB_PORT_CHANGE_ENABLE:
            port_clear_feature(hub->dev, port_id, HUB_FEATURE_C_PORT_ENABLE);
            break;
        case USB_PORT_CHANGE_RESET:
            port_clear_feature(hub->dev, port_id, HUB_FEATURE_C_PORT_RESET);
            break;
        case USB_PORT_CHANGE_OVER_CURRENT:
            port_clear_feature(hub->dev, port_id, HUB_FEATURE_C_PORT_OVER_CURRENT);
            break;
    }
}

static void hub_port_set_feature(usb_hub_t *hub, int port_id, int flag)
{
    switch(flag)
    {
        case USB_PORT_POWER:
            port_set_feature(hub->dev, port_id, HUB_FEATURE_PORT_POWER);
            break;
        case USB_PORT_RESET:
            port_set_feature(hub->dev, port_id, HUB_FEATURE_PORT_RESET);
            break;
    }
}

static void hub_status_change_handler(void *data)
{
    usb_hub_t *hub = data;
    usb_dev_t *dev = hub->dev;

    uint8_t map = *(uint8_t*)dev->buf_virt;
    for(int i = 1; i <= hub->number_of_ports; i++)
    {
        if(map & (1 << i))
        {
            hub_port_status(hub, i);
            usb_core_hub_notify(hub, i);
        }
    }
}

static void hub_init(usb_hub_t *hub)
{
    usb_dev_t *dev = hub->dev;
    usb_endpoint_descriptor_t *ep = 0;

    while(ep = usb_next_descriptor(dev, ep), ep)
    {
        if(ep->bDescriptorType == USB_DESCRIPTOR_ENDPOINT)
        {
            hub->dci = xhci_endpoint_configure(dev, ep);
            xhci_endpoint_set_handler(dev, hub->dci , hub_status_change_handler, hub);
            break;
        }
    }
}

static void hub_wait(usb_hub_t *hub)
{
    usb_dev_t *dev = hub->dev;
    if(hub->dci)
    {
        xhci_transfer_normal(dev, hub->dci, dev->buf_phys, 1);
    }
}

//
// Hub initialization
//

void usb_hub_init(usb_dev_t *dev)
{
    usb_hub_t hub = {
        .depth = dev->parent->depth + 1,
        .speed = dev->port_speed,
        .port_id = dev->port_id,
        .slot_id = dev->slot_id,
        .number_of_ports = 0,

        .parent = dev->parent,
        .dev = dev,
        .hci = dev->parent->hci,
        .dci = 0,

        .hub_init = hub_init,
        .hub_fini  = 0,
        .hub_wait = hub_wait,
        .dev_init = xhci_init_device,
        .dev_fini = xhci_fini_device,
        .port_speed = hub_port_speed,
        .port_status = hub_port_status,
        .port_clear_feature = hub_port_clear_feature,
        .port_set_feature = hub_port_set_feature,
    };

    // Set configuration
    usb_set_configuration(dev, dev->desc_cfg->bConfigurationValue);

    // Set depth
    if(hub.speed >= USB_SUPER_SPEED)
    {
        hub_set_depth(dev, hub.depth);
    }

    // Hub descriptor
    usb_hub_descriptor_t desc;
    hub_get_descriptor(dev, &desc);

    // Downstream ports
    hub.number_of_ports = desc.bNbrPorts;

    // Configure context
    xhci_context_t *ctx = usb_dev_xhci_context(dev);
    xhci_t *xhci = usb_dev_xhci(dev);

    ctx->slot->hub = 1;
    ctx->slot->number_of_ports = hub.number_of_ports;

    if(hub.speed == USB_HIGH_SPEED)
    {
        ctx->slot->ttt = (desc.wHubCharacteristics >> 5) & 0x3;
    }

    xhci_command_evaluate_context(xhci, dev->slot_id, ctx->input_phys);

    // Register
    usb_core_hub_attach(&hub);
}
