#include <kernel/usb/usb-core.h>
#include <kernel/usb/xhci.h>

static int xhci_hub_port_speed(usb_hub_t *hub, int port_id)
{
    uint32_t portsc;
    xhci_port_t *port;
    xhci_t *xhci;

    xhci = hub->hci;
    port = xhci->port + port_id - 1;
    portsc = port->hcp->portsc;

    return (portsc >> 10) & 0xF;
}

static int xhci_hub_port_status(usb_hub_t *hub, int port_id)
{
    uint32_t portsc, status;
    xhci_port_t *port;
    xhci_t *xhci;

    xhci = hub->hci;
    port = xhci->port + port_id - 1;
    portsc = port->hcp->portsc;
    status = 0;

    if(portsc & PORTSC_CCS)
    {
        status |= USB_PORT_STATUS_CONNECTION;
    }
    if(portsc & PORTSC_PED)
    {
        status |= USB_PORT_STATUS_ENABLE;
    }
    if(portsc & PORTSC_OCA)
    {
        status |= USB_PORT_STATUS_OVER_CURRENT;
    }
    if(portsc & PORTSC_PR)
    {
        status |= USB_PORT_STATUS_RESET;
    }
    if(portsc & PORTSC_PP)
    {
        status |= USB_PORT_STATUS_POWER;
    }

    if(portsc & PORTSC_CSC)
    {
        status |= USB_PORT_CHANGE_CONNECTION;
    }
    if(portsc & PORTSC_PEC)
    {
        status |= USB_PORT_CHANGE_ENABLE;
    }
    if(portsc & PORTSC_OCC)
    {
        status |= USB_PORT_CHANGE_OVER_CURRENT;
    }
    if(portsc & PORTSC_PRC)
    {
        status |= USB_PORT_CHANGE_RESET;
    }

    return status;
}

static void xhci_hub_port_clear_feature(usb_hub_t *hub, int port_id, int flag)
{
    uint32_t portsc;
    xhci_port_t *port;
    xhci_t *xhci;

    xhci = hub->hci;
    port = xhci->port + port_id - 1;
    portsc = port->hcp->portsc;

    if(port->major == 2)
    {
        portsc &= 0xFF89FFED;
    }
    else
    {
        portsc &= 0x7F01FFED;
    }

    switch(flag)
    {
        case USB_PORT_POWER:
            portsc &= ~PORTSC_PP;
            break;
        case USB_PORT_ENABLE:
            portsc |= PORTSC_PED;
            break;
        case USB_PORT_CHANGE_CONNECTION:
            portsc |= PORTSC_CSC;
            break;
        case USB_PORT_CHANGE_ENABLE:
            portsc |= PORTSC_PEC;
            break;
        case USB_PORT_CHANGE_RESET:
            portsc |= PORTSC_PRC;
            break;
        case USB_PORT_CHANGE_OVER_CURRENT:
            portsc |= PORTSC_OCC;
            break;
    }

    port->hcp->portsc = portsc;
}

static void xhci_hub_port_set_feature(usb_hub_t *hub, int port_id, int flag)
{
    uint32_t portsc;
    xhci_port_t *port;
    xhci_t *xhci;

    xhci = hub->hci;
    port = xhci->port + port_id - 1;
    portsc = port->hcp->portsc;

    if(port->major == 2)
    {
        portsc &= 0xFF89FFED;
    }
    else
    {
        portsc &= 0x7F01FFED;
    }

    switch(flag)
    {
        case USB_PORT_POWER:
            portsc |= PORTSC_PP;
            break;
        case USB_PORT_RESET:
            portsc |= PORTSC_PR;
            break;
    }

    port->hcp->portsc = portsc;
}

static void xhci_hub_wait(usb_hub_t *hub)
{
    xhci_t *xhci = hub->hci;
    xhci->hub = hub;
}

void xhci_hub_register(xhci_t *xhci)
{
    usb_hub_t hub = {
        .depth = 0,
        .speed = USB_SUPER_SPEED, // This depends on the ports
        .port_id = 0,
        .slot_id = 0,
        .number_of_ports = xhci->maxports,

        .parent = 0,
        .dev = 0,
        .hci = xhci,
        .dci = 0,

        .hub_init = 0,
        .hub_fini = 0,
        .hub_wait = xhci_hub_wait,
        .dev_init = xhci_init_device,
        .dev_fini = xhci_fini_device,
        .port_speed = xhci_hub_port_speed,
        .port_status = xhci_hub_port_status,
        .port_clear_feature = xhci_hub_port_clear_feature,
        .port_set_feature = xhci_hub_port_set_feature,
    };

    usb_core_hub_attach(&hub);
    usb_core_print_tree();
}
