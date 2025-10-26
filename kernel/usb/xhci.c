#include <kernel/usb/usb.h>
#include <kernel/usb/xhci.h>
#include <kernel/mem/vmm.h>
#include <kernel/mem/mmio.h>
#include <kernel/x86/irq.h>
#include <kernel/mem/heap.h>
#include <kernel/time/time.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>
#include <stdlib.h>

xhci_t *usb_dev_xhci(usb_dev_t *dev)
{
    return dev->parent->hci;
}

xhci_context_t *usb_dev_xhci_context(usb_dev_t *dev)
{
    xhci_t *xhci = dev->parent->hci;
    return xhci->ctx + dev->slot_id - 1;
}

static void xhci_handler(int gsi, void *data)
{
    xhci_t *xhci = data;
    xhci_endpoint_t *ep;
    xhci_ring_t *ring;
    xhci_trb_t *trb;
    uint32_t status, iman;
    uint64_t erdp;
    uint32_t type;

    // Acknowledge
    status = xhci->hco->usbsts;
    xhci->hco->usbsts = status;

    // Clear Interrupts
    iman = xhci->hcr->intr[0].iman;
    if(iman & IMAN_IP)
    {
        xhci->hcr->intr[0].iman = iman;
    }

    // Port Change Detected
    if(status & USBSTS_PCD)
    {
        // Set if there is a change on any of the ports
    }

    // Event Interrupt
    if(status & USBSTS_EINT)
    {
        ring = &xhci->event;
        trb = ring->trb + ring->index;
        ring->latest = trb;

        ring->index++;
        if(ring->index == ring->ntrb)
        {
            ring->cycle ^= 1;
            ring->index = 0;
        }

        type = (trb->flags >> 10) & 0x3F;
        if(type == TRB_TYPE_TRANSFER)
        {
            int dci = (trb->flags >> 16) & 0x1F;
            int code = (trb->status >> 24);
            int slot = (trb->flags >> 24);
            if(dci > 1)
            {
                ep = &xhci->ctx[slot-1].ep[dci-1];
                if(ep->handler)
                {
                    ep->handler(ep->data);
                }
            }
        }
        else if(type == TRB_TYPE_PORT_STATUS_CHANGE)
        {
            if(xhci->hub)
            {
                int port = (trb->address >> 24) & 0xFF;
                usb_core_hub_notify(xhci->hub, port);
            }
        }

        erdp = ring->phys + ring->index * sizeof(xhci_trb_t);
        xhci->hcr->intr[0].erdp = erdp | (1 << 3);
    }
}

int xhci_create_command_ring(xhci_ring_t *ring)
{
    uint64_t virt, phys;
    xhci_trb_t *trb;
    int size;

    // Allocate memory
    size = sizeof(xhci_trb_t) * XHCI_MAX_COMMANDS;
    if(ring->phys == 0)
    {
        mmio_alloc_uc_region(size, 64, &virt, &phys);
    }
    else
    {
        phys = ring->phys;
        virt = (uint64_t)ring->trb;
    }

    // Initialize ring
    ring->trb = (void*)virt;
    ring->phys = phys;
    ring->index = 0;
    ring->ntrb = XHCI_MAX_COMMANDS;
    ring->cycle = 1;

    // Zero memory
    memset(ring->trb, 0, size);

    // See section 4.11.5.1 for Link TRB
    trb = ring->trb + ring->ntrb;
    trb->address = phys;
    trb->status = 0;
    trb->flags = (TRB_TYPE_LINK << 10) | TRB_ENT; // TRB_ENT = Toggle Cycle for Link TRB

    return 0;
}

void xhci_allocate_context(usb_dev_t *dev)
{
    xhci_context_t *ctx;
    xhci_t *xhci;
    uint64_t phys, virt;
    int size;

    // Context
    ctx = usb_dev_xhci_context(dev);
    xhci = usb_dev_xhci(dev);

    // Allocate
    size = 2*PAGE_SIZE;
    if(ctx->input_phys == 0)
    {
        mmio_alloc_uc_region(size, 64, &virt, &phys);
    }
    else
    {
        phys = ctx->input_phys;
        virt = (uint64_t)ctx->ctrl;
    }

    // Initialize memory
    memset((void*)virt, 0, size);

    // Context Pointers
    ctx->output_phys = phys + PAGE_SIZE;
    ctx->input_phys = phys;
    ctx->ctrl = (void*)virt;
    ctx->slot = (void*)(virt + xhci->ctxsize);

    for(int i = 0; i < 31; i++)
    {
        ctx->ep[i].ctx = (void*)(virt + (i + 2) * xhci->ctxsize);
        ctx->ep[i].dci = i + 1;
        ctx->ep[i].handler = 0;
    }

    ctx->ep0 = ctx->ep[0].ctx;
    ctx->cmd = &ctx->ep[0].ring;
}

static void xhci_create_device_context(usb_dev_t *dev)
{
    xhci_context_t *ctx;
    xhci_t *xhci;
    usb_hub_t *hub;
    int status;

    // Shortcuts
    ctx = usb_dev_xhci_context(dev);
    xhci = usb_dev_xhci(dev);
    hub = dev->parent;

    // Output context
    xhci->dcba[dev->slot_id] = ctx->output_phys;

    // New command ring
    xhci_create_command_ring(ctx->cmd);

    // Add Context flags (A0 - A31)
    // We only use the first two Device Context data structures
    ctx->ctrl->add_flags = (1 << 1) | (1 << 0);

    // Slot Context
    ctx->slot->entries = 1;
    ctx->slot->speed = dev->port_speed;
    ctx->slot->root_hub_port = dev->root_port_id;
    ctx->slot->intr_target = 0;
    ctx->slot->mtt = 0;

    // For non-root hubs
    if(hub->depth)
    {
        if(dev->port_speed < USB_HIGH_SPEED)
        {
            for(usb_hub_t *h = hub; h; h = h->parent)
            {
                if(h->speed == USB_HIGH_SPEED)
                {
                    ctx->slot->tt_hub_slot_id = h->slot_id;
                    ctx->slot->tt_port = h->port_id;
                    break;
                }
            }
        }

        ctx->slot->route_string = dev->port_id;
        while(hub->depth > 1)
        {
            ctx->slot->route_string = (ctx->slot->route_string << 4) | hub->port_id;
            hub = hub->parent;
        }
    }

    // EP Context 0 (Bidirectional)
    ctx->ep0->tr_dequeue_ptr = ctx->cmd->phys | 1;
    ctx->ep0->ep_type = EP_TYPE_CONTROL;
    ctx->ep0->max_packet_size = xhci_endpoint_max_packet_size(dev);
    ctx->ep0->avg_trb_length = 8;
    ctx->ep0->cerr = 3;

    // Address device
    status = xhci_command_address_device(xhci, dev->slot_id, ctx->input_phys);
    if(status == 1)
    {
        return;
    }

    // Reset device
    status = xhci_command_reset_device(xhci, dev->slot_id, ctx->input_phys);
    if(status != 1 && status != 19)
    {
        kp_error("xhci", "device reset failed (status %d)", status);
        return;
    }
    timer_sleep(25);

    // Address device again
    status = xhci_command_address_device(xhci, dev->slot_id, ctx->input_phys);
    if(status != 1)
    {
        kp_error("xhci", "address device failed (status %d)", status);
    }
}

void xhci_init_device(usb_dev_t *dev)
{
    xhci_context_t *ctx;
    xhci_t *xhci;

    // Enable slot
    xhci = usb_dev_xhci(dev);
    dev->slot_id = xhci_command_enable_slot(xhci);
    if(dev->slot_id == 0)
    {
        return;
    }

    // Allocate slot
    xhci_allocate_context(dev);
    ctx = usb_dev_xhci_context(dev);

    // Debug
    kp_info("xhci", "port%d: speed: %d, slot id: %d", dev->port_id, dev->port_speed, dev->slot_id);

    // Create device context
    xhci_create_device_context(dev);

    // Get partial device descriptor
    usb_get_device_descriptor(dev, 8);

    // Adjust Max Packet Size
    ctx->ep0->max_packet_size = xhci_endpoint_max_packet_size(dev);

    // Evaluate context change
    xhci_command_evaluate_context(xhci, dev->slot_id, ctx->input_phys);

    // Get full device descriptor
    usb_get_device_descriptor(dev, 18);

    dev->bcdUSB = dev->desc_dev->bcdUSB;
    dev->bcdDevice = dev->desc_dev->bcdDevice;
    dev->devClass = dev->desc_dev->bDeviceClass;
    dev->devSubClass = dev->desc_dev->bDeviceSubClass;
    dev->devProtocol = dev->desc_dev->bDeviceProtocol;
    dev->idVendor = dev->desc_dev->idVendor;
    dev->idProduct = dev->desc_dev->idProduct;

    usb_get_string_descriptor(dev, dev->desc_dev->iManufacturer, dev->strVendor);
    usb_get_string_descriptor(dev, dev->desc_dev->iProduct, dev->strProduct);

    // Configuration descriptor
    usb_get_config_descriptor(dev);

    // Check first interface
    usb_interface_descriptor_t *ifc = (void*)(dev->desc_cfg + 1);

    if(dev->devClass == 0)
    {
        dev->devClass = ifc->bInterfaceClass;
    }
    if(dev->devSubClass == 0)
    {
        dev->devSubClass = ifc->bInterfaceSubClass;
    }
    if(dev->devProtocol == 0)
    {
        dev->devProtocol = ifc->bInterfaceProtocol;
    }
}

void xhci_fini_device(usb_dev_t *dev)
{
    xhci_t *xhci;
    xhci = usb_dev_xhci(dev);
    xhci_command_disable_slot(xhci, dev->slot_id);
}

static int xhci_init_device_context(xhci_t *xhci)
{
    uint64_t ctx_virt, ctx_phys;
    uint64_t scp_virt, scp_phys;
    int size;

    // Configure "Device Context Base Address Array Pointer"
    // Uses at most 256*8B = 2 KiB of memory

    size = 2048;
    if(mmio_alloc_uc_region(size, 64, &ctx_virt, &ctx_phys))
    {
        kp_error("xhci", "alloc uc failed");
        return -ENOMEM;
    }

    xhci->hco->dcbaap = ctx_phys;
    xhci->dcba = (void*)ctx_virt;
    memset(xhci->dcba, 0, size);

    // Configure "Scratchpad Buffer Array"
    // The location of the Scratchpad Buffer Array is defined by entry 0 of the Device Context Base Address Array
    // All devices are 1-indexed, so the first element is reserved for this purpose

    if(xhci->maxscpbufs == 0)
    {
        return 0;
    }

    size = xhci->maxscpbufs * PAGE_SIZE;
    if(mmio_alloc_uc_region(size, 64, &scp_virt, &scp_phys))
    {
        kp_error("xhci", "alloc uc failed");
        return -ENOMEM;
    }

    memset((void*)scp_virt, 0, size);
    xhci->dcba[0] = scp_phys;

    return 0;
}

static int xhci_init_command_ring(xhci_t *xhci)
{
    int status;

    status = xhci_create_command_ring(&xhci->cmd);
    if(status)
    {
        kp_error("xhci", "failed to allocate command ring");
        return status;
    }

    xhci->hco->crcr = (xhci->cmd.phys | CRCR_RCS);
    return 0;
}

static int xhci_init_event_ring(xhci_t *xhci)
{
    uint64_t virt, phys;
    int size;

    size = sizeof(xhci_erste_t) + sizeof(xhci_trb_t) * XHCI_MAX_EVENTS;
    if(mmio_alloc_uc_region(size, 64, &virt, &phys))
    {
        kp_error("xhci", "alloc uc failed");
        return -ENOMEM;
    }

    xhci->erst = (void*)virt;
    memset(xhci->erst, 0, size);

    xhci->event.trb = (void*)(xhci->erst + 1);
    xhci->event.phys = phys + sizeof(xhci_erste_t);
    xhci->event.index = 0;
    xhci->event.latest = 0;
    xhci->event.ntrb = XHCI_MAX_EVENTS;
    xhci->event.cycle = 0;

    xhci->erst->rsba = xhci->event.phys;
    xhci->erst->rss = XHCI_MAX_EVENTS;

    xhci->hcr->intr[0].erstsz = 1;
    xhci->hcr->intr[0].erdp = xhci->event.phys | (1 << 3);
    xhci->hcr->intr[0].erstba = phys;

    return 0;
}

static int xhci_init_extended_capabilities(xhci_t *xhci)
{
    uint64_t base, xecp;
    uint32_t temp;

    xecp = (xhci->hcc->hccparams1 >> 16);
    base = (uint64_t)xhci->hcc;

    while(xecp)
    {
        base = base + (xecp << 2);
        temp = *(uint32_t*)base;
        xecp = ((temp & 0xFF00) >> 8);

        // ID 2 = Supported Protocol
        if((temp & 0xFF) != 2)
        {
            continue;
        }

        int major = (temp >> 24) & 0xF;
        int minor = (temp >> 16) & 0xF;

        temp = *(uint32_t*)(base+8);
        uint32_t offset = (temp & 0xFF);         // Compatible Port Offset
        uint32_t count  = ((temp >> 8) & 0xFF);  // Compatible Port Count
        // uint32_t psic   = ((temp >> 28) & 0xF);  // Protocol Speed ID Count !!!! 7.2.2.1.2

        if(count == 0 || offset == 0)
        {
            continue;
        }

        // printk(INFO, "xhci", "ver: %d.%d, offset: %d, count: %d, psic: %d", major, minor, offset, count, psic);
        // for(int i = 0; i < psic; i++)
        // {
        //     temp = *(uint32_t*)(base+0x10+4*i);
        //     int psiv = (temp & 0xF); // Protocol Speed ID Value
        //     int psie = ((temp >> 4) & 0x3); // Protocol Speed ID Exponent
        //     int psim = (temp >> 16); // Protocol Speed ID Mantissa
        //     int plt = (temp >> 6) & 0x3; // PSI Type
        //     uint64_t speed  = psim;
        //     for(int i = 0; i < psie; i++)
        //     {
        //         speed *= 1024;
        //     }
        //     printk(INFO, "xhci", "psiv: %d, psie: %d, psim: %d, speed: %lu, plt: %d", psiv, psie, psim, speed, plt);
        // }

        for(int i = offset; i < offset + count; i++)
        {
            xhci->port[i-1].major = major;
            xhci->port[i-1].minor = minor;
        }
    }

    return 0;
}

static int xhci_alloc_irq_vectors(pci_dev_t *dev, xhci_t *xhci)
{
    int status, vector;

    status = pci_alloc_irq_vectors(dev, 1, 1);
    if(status < 0)
    {
        kp_error("xhci", "failed to allocated IRQ vectors (status %d)", status);
        return status;
    }

    vector = pci_irq_vector(dev, 0);
    irq_request(vector, xhci_handler, xhci);

    return 0;
}

void xhci_init(pci_dev_t *pcidev, uint64_t address)
{
    uint64_t hcs1int, hcs2int;
    uint32_t caplength, version;
    xhci_hcsparams1_t *hcs1 = (void*)&hcs1int;
    xhci_hcsparams2_t *hcs2 = (void*)&hcs2int;

    // Allocate memory
    xhci_t *xhci = kzalloc(sizeof(xhci_t));

    // Registers
    xhci->hcc = (void*)address;
    caplength = (xhci->hcc->capver & 0xFF);
    version   = (xhci->hcc->capver >> 16);
    hcs1int   = xhci->hcc->hcsparams1;
    hcs2int   = xhci->hcc->hcsparams2;

    xhci->hco = (void*)(address + caplength);
    xhci->hcr = (void*)(address + xhci->hcc->rtsoff);
    xhci->db  = (void*)(address + xhci->hcc->dboff);

    // Properties
    if((xhci->hcc->hccparams1 & CAP_AC64) == 0)
    {
        kp_error("xhci", "64-bit addressing not supported");
        return;
    }

    if(xhci->hcc->hccparams1 & CAP_CSZ)
    {
        xhci->ctxsize = 0x40;
    }
    else
    {
        xhci->ctxsize = 0x20;
    }

    xhci->version    = version;
    xhci->maxports   = hcs1->maxports;
    xhci->maxints    = hcs1->maxintrs;
    xhci->maxslots   = hcs1->maxslots;
    xhci->maxerst    = (1 << hcs2->erstmax);
    xhci->maxscpbufs = (hcs2->maxscpbufshi << 5) | hcs2->maxscpbufslo;

    if(xhci->maxslots > XHCI_MAX_SLOTS)
    {
        xhci->maxslots = XHCI_MAX_SLOTS;
    }

    if((xhci->hco->pagesize & 1) == 0)
    {
        kp_error("xhci", "xhci controller does not support 4K page size");
        return;
    }

    kp_info("xhci", "maxports %d, maxintrs %d, maxslots %d, maxerst %d, maxscpbufs %d", xhci->maxports, xhci->maxints, xhci->maxslots, xhci->maxerst, xhci->maxscpbufs);

    // Allocate ports
    xhci->port = kcalloc(xhci->maxports, sizeof(xhci_port_t));
    for(int i = 0; i < xhci->maxports; i++)
    {
        xhci->port[i].hcp = xhci->hco->port + i;
    }

    // Allocate slots
    xhci->ctx = kcalloc(xhci->maxslots, sizeof(xhci_context_t));

    // Extended capabilities
    xhci_init_extended_capabilities(xhci);

    // Stop running controller
    if(xhci->hco->usbcmd & USBCMD_RS)
    {
        xhci->hco->usbcmd &= ~USBCMD_RS;
        timer_sleep(50);
    }

    if((xhci->hco->usbsts & USBSTS_HCH) == 0)
    {
        kp_error("xhci", "controller not halted, aborting: %x", xhci->hco->usbsts);
        return;
    }

    // Reset
    xhci->hco->usbcmd |= USBCMD_HCRST;
    timer_sleep(50);
    kp_info("xhci", "reset finished: usbcmd %x usbsts %x", xhci->hco->usbcmd, xhci->hco->usbsts);

    // Configure "Max Device Slots Enabled"
    xhci->hco->config = xhci->maxslots;

    // Device Context
    xhci_init_device_context(xhci);

    // Command Ring
    xhci_init_command_ring(xhci);

    // Event Ring
    xhci_init_event_ring(xhci);

    // Enable interrupts
    xhci_alloc_irq_vectors(pcidev, xhci);
    xhci->hcr->intr[0].iman |= (IMAN_IE | IMAN_IP);

    // Start controller
    xhci->hco->usbcmd |= (USBCMD_RS | USBCMD_INTE | USBCMD_HSEE);
    timer_sleep(50);

    if(xhci->hco->usbsts & (USBSTS_HCH | USBSTS_CNR))
    {
        kp_error("xhci", "hco still halted");
        return;
    }

    // Register
    xhci_hub_register(xhci);
}
