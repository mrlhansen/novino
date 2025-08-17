#include <kernel/usb/usb.h>
#include <kernel/usb/xhci.h>
#include <string.h>


// Iterate through all descriptors returned together with the configuration descriptor. The input pointer should
// either be zero (to start the iteration) or a pointer to the last returned descriptor. The function will iterate
// through the descriptors until the end is reached.
void *usb_next_descriptor(usb_dev_t *dev, void *ptr)
{
    usb_config_descriptor_t *cfg;
    uint64_t base, end;

    cfg = dev->desc_cfg;
    base = (uint64_t)cfg;
    end = base + cfg->wTotalLength;

    if(ptr)
    {
        base = (uint64_t)ptr;
    }

    cfg = (void*)base;
    base += cfg->bLength;
    cfg = (void*)base;

    if(base < end)
    {
        if(cfg->bDescriptorType)
        {
            return cfg;
        }
    }

    return 0;
}


// Iterate through all interface descriptors. The input pointer should either be zero (to start the iteration) or a
// pointer to the last returned interface descriptor. The function will iterate through the descriptors until the
// end is reached.
void *find_interface_descriptor(usb_dev_t *dev, void *ptr)
{
    usb_endpoint_descriptor_t *ifd = ptr;

    while(ifd = usb_next_descriptor(dev, ifd), ifd)
    {
        if(ifd->bDescriptorType == USB_DESCRIPTOR_INTERFACE)
        {
            return ifd;
        }
    }

    return 0;
}


// Iterate through all endpoint descriptors with a given type and direction. The input pointer should either be an
// interface descriptor or a pointer to the last returned endpoint descriptor. The function will iterate through the
// endpoint descriptors until another interface descriptor is found or the end is reached.
void *find_endpoint_descriptor(usb_dev_t *dev, void *ptr, int type, int dir)
{
    usb_endpoint_descriptor_t *epd = ptr;
    int edir, etype;

    while(epd = usb_next_descriptor(dev, epd), epd)
    {
        if(epd->bDescriptorType == USB_DESCRIPTOR_ENDPOINT)
        {
            edir = (epd->bEndpointAddress >> 7);
            etype = (epd->bmAttributes & 0x3);
            if((edir == dir) && (etype == type))
            {
                return epd;
            }
        }
        else if(epd->bDescriptorType == USB_DESCRIPTOR_INTERFACE)
        {
            return 0;
        }
    }

    return 0;
}

void *usb_request_submit(usb_dev_t *dev, usb_request_t *req, void *data)
{
    void *buf = (void*)dev->buf_virt;

    if(req->wLength == 0)
    {
        xhci_transfer_control(dev, req, 0, 0, TRT_NO_DATA);
        return 0;
    }
    else if(req->bmRequestType & USB_BM_REQUEST_DEVICE_TO_HOST)
    {
        xhci_transfer_control(dev, req, dev->buf_phys, req->wLength, TRT_IN_DATA);
        return buf;
    }
    else
    {
        memcpy(buf, data, req->wLength); // check buffer size
        xhci_transfer_control(dev, req, dev->buf_phys, req->wLength, TRT_OUT_DATA);
        return 0;
    }

    return 0;
}

void usb_get_device_descriptor(usb_dev_t *dev, int length)
{
    usb_device_descriptor_t *desc;

    // Send request
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_STANDARD | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_GET_DESCRIPTOR,
        .wValue        = (USB_DESCRIPTOR_DEVICE << 8),
        .wLength       = length,
        .wIndex        = 0,
    };
    desc = usb_request_submit(dev, &req, 0);

    // Copy descriptor
    memcpy(dev->desc_dev, desc, desc->bLength);
}

void usb_get_string_descriptor(usb_dev_t *dev, int index, char *str)
{
    usb_string_descriptor_t *desc;

    // No string available
    if(index == 0)
    {
        str[0] = '\0';
        return;
    }

    // Send request
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_STANDARD | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_GET_DESCRIPTOR,
        .wValue        = (USB_DESCRIPTOR_STRING << 8) | index,
        .wLength       = 0xFF,
        .wIndex        = 0x0409, // LANGID = English US - maybe this also needs to be an argument
    };
    desc = usb_request_submit(dev, &req, 0);

    // Copy descriptor
    if(str == 0)
    {
        return;
    }

    for(int i = 0; i < (desc->bLength-2)/2; i++)
    {
        str[i] = desc->bString[i];
        str[i+1] = '\0';
    }
}

void usb_get_config_descriptor(usb_dev_t *dev)
{
    usb_config_descriptor_t *desc;

    // Send request
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_DEVICE_TO_HOST | USB_BM_REQUEST_STANDARD | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_GET_DESCRIPTOR,
        .wValue        = (USB_DESCRIPTOR_CONFIGURATION << 8),
        .wLength       = 0xFF,
        .wIndex        = 0,
    };
    desc = usb_request_submit(dev, &req, 0);

    // TODO: This can actually be more than 256 bytes long!
    // Copy descriptor
    memcpy(dev->desc_cfg, desc, desc->wTotalLength);
}

void usb_set_configuration(usb_dev_t *dev, int value)
{
    usb_request_t req = {
        .bmRequestType = USB_BM_REQUEST_HOST_TO_DEVICE | USB_BM_REQUEST_STANDARD | USB_BM_REQUEST_DEVICE,
        .bRequest      = USB_B_REQUEST_SET_CONFIGURATION,
        .wValue        = value,
        .wLength       = 0,
        .wIndex        = 0,
    };
    usb_request_submit(dev, &req, 0);
}
