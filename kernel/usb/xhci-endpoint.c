#include <kernel/usb/xhci.h>
#include <kernel/debug.h>
#include <stdlib.h>

int xhci_endpoint_max_packet_size(usb_dev_t *dev)
{
    int mps = 0;

    // Use "Device Descriptor" when available
    mps = dev->desc_dev->bMaxPacketSize0;
    if(mps > 0)
    {
        if(dev->port_speed >= USB_SUPER_SPEED)
        {
            mps = (1 << mps);
        }
        return mps;
    }

    // Use defaults
    switch(dev->port_speed)
    {
        case USB_LOW_SPEED:
            mps = 8;
            break;
        case USB_FULL_SPEED:
        case USB_HIGH_SPEED:
            mps = 64;
            break;
        case USB_SUPER_SPEED:
        case USB_SUPER_SPEED_PLUS:
            mps = 512;
            break;
        default:
            kp_error("xhci", "unknown speed :(");
            break;
    }

    return mps;
}

int xhci_endpoint_interval(usb_dev_t *dev, usb_endpoint_descriptor_t *epd)
{
    int type = (epd->bmAttributes & 0x3);
    int interval = 0;

    // Section 6.2.3.6
    if(type == USB_ENDPOINT_TYPE_INTERRUPT)
    {
        // Convert to 3-10 for full-speed and low-speed
        if(dev->port_speed < USB_LOW_SPEED)
        {
            int target = max(epd->bInterval, 1);
            target = (1000 * target) / 125;
            interval = -1;

            while(target)
            {
                interval++;
                target /= 2;
            }
        }
        else // Convert to 0-15 for high-speed and above
        {
            interval = min(max(epd->bInterval, 1), 16) - 1;
        }
    }
    else if(type == USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        // Convert to 0-15
        interval = min(max(epd->bInterval, 1), 16) - 1;

        // Full-speed should be 3-18
        if(dev->port_speed == USB_FULL_SPEED)
        {
            interval = interval + 3;
        }
    }

    return interval;
}

int xhci_endpoint_max_esit(usb_dev_t *dev, usb_endpoint_descriptor_t *epd, int mps, int mbs)
{
    usb_endpoint_companion_descriptor_t *epcd;
    int type = (epd->bmAttributes & 0x3);
    int esit = 0;

    // Not relevant
    if(type == USB_ENDPOINT_TYPE_BULK)
    {
        return 0;
    }

    if(type == USB_ENDPOINT_TYPE_CONTROL)
    {
        return 0;
    }

    // Section 4.14.2
    if(dev->port_speed >= USB_SUPER_SPEED)
    {
        epcd = (void*)(epd + 1);
        if(epcd->bmAttributes & 0x80)
        {
            // Check the SuperSpeedPlus Isochronous Endpoint Companion
        }
        else
        {
            esit = epcd->wBytesPerInterval;
        }
    }
    else
    {
        esit = mps * (mbs + 1);
    }

    return esit;
}

int xhci_endpoint_configure(usb_dev_t *dev, usb_endpoint_descriptor_t *epd)
{
    usb_endpoint_companion_descriptor_t *epcd;
    xhci_endpoint_context_t *epctx;
    xhci_context_t *ctx;
    xhci_ring_t *ring;
    xhci_t *xhci;
    uint32_t flags;

    int num = (epd->bEndpointAddress & 0xF);
    int dir = (epd->bEndpointAddress >> 7); // 0 = out, 1 = in
    int type = (epd->bmAttributes & 0x3);

    int ep_type = 0;
    int cerr = 3;
    int dci = 0; // Page 462
    int avg_trb_length = 0; // Page 258
    int max_burst_size = 0; // Section 4.8.2
    int max_packet_size = epd->wMaxPacketSize;
    int max_esit = 0;

    if(type == USB_ENDPOINT_TYPE_CONTROL)
    {
        dci = 2 * (num + 1) - 1;
        ep_type = EP_TYPE_CONTROL;
        avg_trb_length = 8;
    }
    else
    {
        dci = 1 + dir + (2 * num) - 1;
        epcd = (void*)(epd + 1);

        if(type == USB_ENDPOINT_TYPE_BULK)
        {
            ep_type = dir ? EP_TYPE_BULK_IN : EP_TYPE_BULK_OUT;
            avg_trb_length = 3072;
            cerr = 0;
            if(dev->port_speed >= USB_SUPER_SPEED)
            {
                max_burst_size = epcd->bMaxBurst;
            }
            // MaxPStreams, HID, LSA
        }
        else
        {
            max_burst_size = (max_packet_size & 0x1800) >> 11;
            max_packet_size = (max_packet_size & 0x7FF);

            if(type == USB_ENDPOINT_TYPE_ISOCHRONOUS)
            {
                ep_type = dir ? EP_TYPE_ISOCHRONOUS_IN : EP_TYPE_ISOCHRONOUS_OUT;
                avg_trb_length = 3072;
                // Mult
            }
            else
            {
                ep_type = dir ? EP_TYPE_INTERRUPT_IN : EP_TYPE_INTERRUPT_OUT;
                avg_trb_length = 1024;
                cerr = 0;
            }
        }
    }

    max_esit = xhci_endpoint_max_esit(dev, epd, max_packet_size, max_burst_size);

    ctx = usb_dev_xhci_context(dev);
    xhci = usb_dev_xhci(dev);

    ring = &ctx->ep[dci-1].ring;
    epctx = ctx->ep[dci-1].ctx;
    flags = ctx->ctrl->add_flags;

    xhci_create_command_ring(ring);

    epctx->interval = xhci_endpoint_interval(dev, epd);
    epctx->ep_type = ep_type;
    epctx->cerr = cerr;
    epctx->max_packet_size = max_packet_size;
    epctx->max_burst_size = max_burst_size;
    epctx->max_p_streams = 0;
    epctx->mult = 0;
    epctx->hid = 0;
    epctx->tr_dequeue_ptr = ring->phys | 1;
    epctx->avg_trb_length = avg_trb_length;
    epctx->max_payload_lo = (max_esit & 0xFFFF);
    epctx->max_payload_hi = (max_esit >> 16) & 0xFF;

    // Update index of last valid endpoint
    if(ctx->slot->entries < dci)
    {
        ctx->ctrl->add_flags = 0x3;
        ctx->slot->entries = dci;
        xhci_command_evaluate_context(xhci, dev->slot_id, ctx->input_phys);
    }

    // Configure endpoint
    flags |= (1 << dci);
    flags &= ~0x2; // A1 must be zero when calling configure endpoint (Page 116)
    ctx->ctrl->add_flags = flags;
    xhci_command_configure_endpoint(xhci, dev->slot_id, ctx->input_phys);

    return dci;
}

void xhci_endpoint_set_handler(usb_dev_t *dev, int dci, xhci_handler_t handler, void *data)
{
    xhci_endpoint_t *ep;
    xhci_context_t *ctx;

    ctx = usb_dev_xhci_context(dev);
    ep = ctx->ep + dci - 1;

    if(ep->ctx->ep_type == EP_TYPE_INTERRUPT_IN)
    {
        ep->handler = handler;
        ep->data = data;
    }
}
