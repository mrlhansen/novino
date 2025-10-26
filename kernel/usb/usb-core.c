#include <kernel/usb/usb-core.h>
#include <kernel/usb/usb-hub.h>
#include <kernel/usb/usb-hid.h>
#include <kernel/time/time.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/mmio.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>
#include <string.h>
#include <stdio.h>

static usb_hub_t *root = 0;

static void usb_core_allocate_device(usb_dev_t *dev)
{
    uint64_t phys, virt;

    // Allocate Transfer Buffer
    if(dev->buf_phys)
    {
        phys = dev->buf_phys;
        virt = dev->buf_virt;
    }
    else
    {
        mmio_alloc_uc_region(PAGE_SIZE, 64, &virt, &phys);
    }

    // Zero
    memset(dev, 0, sizeof(*dev));

    // Transfer Buffer
    memset((void*)virt, 0, PAGE_SIZE);
    dev->buf_phys = phys;
    dev->buf_virt = virt;
    dev->buf_size = PAGE_SIZE;

    // Descriptors
    dev->desc_dev = (void*)dev->desc_buf;
    dev->desc_cfg = (void*)(dev->desc_dev + 1);
}

void usb_core_port_change(usb_hub_t *hub, int port_id)
{
    int status;
    status = hub->port_status(hub, port_id);

    if(status & USB_PORT_CHANGE_RESET)
    {
        hub->port_clear_feature(hub, port_id, USB_PORT_CHANGE_RESET);
    }

    if(status & USB_PORT_CHANGE_ENABLE)
    {
        hub->port_clear_feature(hub, port_id, USB_PORT_CHANGE_ENABLE);
    }

    if(status & USB_PORT_CHANGE_CONNECTION)
    {
        hub->port_clear_feature(hub, port_id, USB_PORT_CHANGE_CONNECTION);

        if(status & USB_PORT_CHANGE_RESET)
        {
            return;
        }

        if(status & USB_PORT_STATUS_CONNECTION)
        {
            // Attach
            usb_core_port_init(hub, port_id);
            usb_core_print_tree();
        }
        else
        {
            // Detach
            usb_core_port_fini(hub, port_id);
            usb_core_print_tree();
        }
    }
}

void usb_core_port_fini(usb_hub_t *hub, int port_id)
{
    usb_dev_t *dev;
    dev = hub->port + port_id - 1;

    if(dev->present == 0)
    {
        return;
    }

    if(dev->devClass == USB_CLASS_HUB)
    {
        for(usb_hub_t *h = hub->child; h; h = h->next)
        {
            if(h->dev == dev)
            {
                usb_core_hub_detach(h);
                break;
            }
        }
    }

    hub->dev_fini(dev);
    dev->present = 0;
}

void usb_core_port_init(usb_hub_t *hub, int port_id)
{
    usb_dev_t *dev;
    int status;

    // Power
    status = hub->port_status(hub, port_id);
    if((status & USB_PORT_STATUS_POWER) == 0)
    {
        hub->port_set_feature(hub, port_id, USB_PORT_POWER);
        timer_sleep(100); // bPwrOn2PwrGood
    }

    // Connection
    status = hub->port_status(hub, port_id);
    if((status & USB_PORT_STATUS_CONNECTION) == 0)
    {
        return;
    }

    // Reset
    if((status & USB_PORT_STATUS_ENABLE) == 0)
    {
        hub->port_set_feature(hub, port_id, USB_PORT_RESET);
        do {
            status = hub->port_status(hub, port_id);
        } while (status & USB_PORT_STATUS_RESET);
    }

    // Acknowledge
    usb_core_port_change(hub, port_id);

    // Device Information
    dev = hub->port + port_id - 1;
    usb_core_allocate_device(dev);

    dev->present = 1;
    dev->parent = hub;
    dev->port_id = port_id;
    dev->port_speed = hub->port_speed(hub, port_id);

    if(hub->depth == 0)
    {
        dev->root_port_id = port_id;
    }
    else
    {
        usb_hub_t *h;
        for(h = hub; h->depth > 1; h = h->parent);
        dev->root_port_id = h->port_id;
    }

    // Initialize Device
    hub->dev_init(dev);

    // Device class driver
    switch(dev->devClass)
    {
        case USB_CLASS_HID:
            usb_hid_init(dev);
            break;
        case USB_CLASS_HUB:
            usb_hub_init(dev);
            break;
        default:
            break;
    }
}

static void usb_core_hub_worker(usb_hub_t *hub)
{
    uint32_t status, bit;
    while(1)
    {
        if(hub->wrk.detach)
        {
            kfree(hub);
            thread_exit();
        }

        acquire_lock(&hub->wrk.lock);
        status = hub->wrk.status;
        release_lock(&hub->wrk.lock);

        if(status == 0)
        {
            if(hub->hub_wait)
            {
                hub->hub_wait(hub);
            }
            thread_wait();
            continue;
        }

        for(int i = 1; i <= hub->number_of_ports; i++)
        {
            bit = (1 << i);
            if(status & bit)
            {
                usb_core_port_change(hub, i);
            }
        }

        acquire_lock(&hub->wrk.lock);
        hub->wrk.status &= ~status;
        release_lock(&hub->wrk.lock);
    }
}

void usb_core_hub_notify(usb_hub_t *hub, int port_id)
{
    acquire_lock(&hub->wrk.lock);
    hub->wrk.status |= (1 << port_id);
    release_lock(&hub->wrk.lock);
    thread_signal(hub->wrk.thread);
}

void usb_core_hub_detach(usb_hub_t *hub)
{
    usb_hub_t *p;

    // Detach all devices
    for(int i = 0; i < hub->number_of_ports; i++)
    {
        if(hub->port[i].present)
        {
            usb_core_port_fini(hub, i+1);
        }
    }

    // Free buffer for all ports
    // todo

    // Unlink from parent
    p = hub->parent;
    if(p->child == hub)
    {
        p->child = hub->next;
    }
    else
    {
        p = p->child;
        while(p)
        {
            if(p->next == hub)
            {
                p->next = hub->next;
                break;
            }
            p = p->next;
        }
    }

    // Exit worker thread
    hub->wrk.detach = 1;
    thread_unblock(hub->wrk.thread);
}

void usb_core_hub_attach(usb_hub_t *ptr)
{
    usb_hub_t *hub;
    int size;

    // Allocate memory
    size = sizeof(usb_hub_t) + ptr->number_of_ports * sizeof(usb_dev_t);
    hub = kzalloc(size);
    if(hub == 0)
    {
        return;
    }

    *hub = *ptr;
    hub->port = (void*)(hub+1);

    // Link to parent
    if(hub->parent == 0)
    {
        if(root == 0)
        {
            hub->next = 0;
            hub->child = 0;
            hub->bus_id = 0;
            root = hub;
        }
        else
        {
            hub->next = root;
            hub->child = 0;
            hub->bus_id = root->bus_id + 1;
            root = hub;
        }
    }
    else
    {
        hub->next = hub->parent->child;
        hub->parent->child = hub;
        hub->bus_id = hub->parent->bus_id;
    }

    // Hub init
    if(hub->hub_init)
    {
        hub->hub_init(hub);
    }

    // Ports init
    for(int i = 0; i < hub->number_of_ports; i++)
    {
        usb_core_port_init(hub, i+1);
    }

    // Worker thread
    char name[32];
    sprintf(name, "xhci.%d.%d", hub->bus_id, hub->port_id);

    hub->wrk.thread = kthreads_create(name, usb_core_hub_worker, hub, TPR_SRT);
    hub->wrk.status = 0;
    hub->wrk.detach = 0;
    hub->wrk.lock = 0;

    kthreads_run(hub->wrk.thread);
}

static void usb_hub_print_hub(usb_hub_t *hub)
{
    char buf[64];
    char tmp[64];
    int path[5];

    for(usb_hub_t *h = hub; h; h = h->parent)
    {
        if(h->depth)
        {
            path[h->depth] = h->port_id;
        }
        else
        {
            path[h->depth] = h->bus_id;
        }
    }

    buf[0] = '\0';
    for(int i = 0; i <= hub->depth; i++)
    {
        if(i == 0)
        {
            sprintf(tmp, "%d:", path[i]);
        }
        else
        {
            sprintf(tmp, "%d.", path[i]);
        }
        strcat(buf, tmp);
    }

    for(int i = 0; i < hub->number_of_ports; i++)
    {
        if(hub->port[i].present)
        {
            usb_dev_t *p = &hub->port[i];
            sprintf(tmp, "%s%d", buf,p->port_id);
            kp_info("hub", "%- 8s (%x.%x, %04x:%04x) (%s : %s)", tmp, (p->bcdUSB >> 8),(p->bcdUSB >> 4) & 0xF, p->idVendor, p->idProduct, p->strVendor, p->strProduct);
        }
    }
}

static void usb_subsystem_print_level(usb_hub_t *hub)
{
    usb_hub_t *r = hub;
    while(r)
    {
        usb_hub_print_hub(r);
        if(r->child)
        {
            usb_subsystem_print_level(r->child);
        }
        r = r->next;
    }
}

void usb_core_print_tree()
{
    usb_subsystem_print_level(root);
}
