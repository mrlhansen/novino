#include <kernel/usb/usb.h>
#include <kernel/usb/xhci.h>
#include <kernel/time/time.h>
#include <kernel/debug.h>
#include <kernel/x86/strace.h> // temporary

static xhci_trb_t *xhci_poll_event(xhci_t *xhci, int type)
{
    xhci_trb_t *trb;
    int slot, code;

    uint64_t now = system_timestamp();
    uint64_t end = now + 100000000; // 100ms

    while(now < end)
    {
        trb = xhci->event.latest;
        if(trb)
        {
            code = (trb->flags >> 10) & 0x3F;
            if(code == type)
            {
                xhci->event.latest = 0;
                code = (trb->status >> 24);
                slot = (trb->flags >> 24);
                if(code != 1 && code != 13)
                {
                    //  1 = Success
                    // 13 = Short Packet (not considered an error)
                    kp_error("xhci", "event completion code: %d - slot: %d", code, slot);
                }
                return trb;
            }
        }
        now = system_timestamp();
    }

    kp_error("xhci", "polling timed out for: %d", type);
    strace(5);
    // while (1);
    return 0;
}

static void xhci_ring_doorbell(xhci_t *xhci, int slot, int endpoint)
{
    xhci->db->value[slot] = endpoint;
}

static void xhci_command_submit(xhci_ring_t *ring, uint64_t addr, uint32_t status, uint32_t flags)
{
    xhci_trb_t *trb;

    flags &= ~TRB_CYCLE;
    flags |= ring->cycle;

    trb = ring->trb + ring->index;
    trb->address = addr;
    trb->status = status;
    trb->flags = flags;

    ring->index++;
    if(ring->index == ring->ntrb)
    {
        trb = ring->trb + ring->ntrb;

        trb->flags &= ~(TRB_CHAIN | TRB_CYCLE);
        trb->flags |= ring->cycle;
        if(flags & TRB_CHAIN)
        {
            trb->flags |= TRB_CHAIN;
        }

        ring->cycle ^= 1;
        ring->index = 0;
    }
}

void xhci_command_evaluate_context(xhci_t *xhci, int slot, uint64_t phys)
{
    xhci_command_submit(&xhci->cmd, phys, 0, (slot << 24) | (TRB_TYPE_EVALUATE_CONTEXT << 10));
    xhci_ring_doorbell(xhci, 0, 0);
    xhci_poll_event(xhci, TRB_TYPE_COMMAND_COMPLETION);
}

void xhci_command_configure_endpoint(xhci_t *xhci, int slot, uint64_t phys)
{
    xhci_command_submit(&xhci->cmd, phys, 0, (slot << 24) | (TRB_TYPE_CONFIGURE_ENDPOINT << 10));
    xhci_ring_doorbell(xhci, 0, 0);
    xhci_poll_event(xhci, TRB_TYPE_COMMAND_COMPLETION);
}

void xhci_command_address_device(xhci_t *xhci, int slot, uint64_t phys)
{
    xhci_command_submit(&xhci->cmd, phys, 0, (slot << 24) | (TRB_TYPE_ADDRESS_DEVICE << 10));
    xhci_ring_doorbell(xhci, 0, 0);
    xhci_poll_event(xhci, TRB_TYPE_COMMAND_COMPLETION);
}

int xhci_command_enable_slot(xhci_t *xhci)
{
    xhci_trb_t *event;

    xhci_command_submit(&xhci->cmd, 0, 0, (0 << 16) | (TRB_TYPE_ENABLE_SLOT << 10));
    xhci_ring_doorbell(xhci, 0, 0);

    event = xhci_poll_event(xhci, TRB_TYPE_COMMAND_COMPLETION);
    if(event == 0)
    {
        return 0;
    }

    return (event->flags >> 24) & 0xFF;
}

void xhci_command_disable_slot(xhci_t *xhci, int slot)
{
    xhci_command_submit(&xhci->cmd, 0, 0, (slot << 24) | (TRB_TYPE_DISABLE_SLOT << 10));
    xhci_ring_doorbell(xhci, 0, 0);
    xhci_poll_event(xhci, TRB_TYPE_COMMAND_COMPLETION);
}

void xhci_command_noop(xhci_t *xhci)
{
    xhci_command_submit(&xhci->cmd, 0, 0, (TRB_TYPE_CMD_NOOP << 10) | TRB_IOC);
    xhci_ring_doorbell(xhci, 0, 0);
    xhci_poll_event(xhci, TRB_TYPE_COMMAND_COMPLETION);
}

// Section 6.4.1.2
void xhci_transfer_control(usb_dev_t *dev, usb_request_t *req, uint64_t phys, int size, int trt)
{
    uint32_t status, flags;
    uint64_t addr;

    uint64_t wValue = req->wValue;
    uint64_t bRequest = req->bRequest;
    uint64_t bmRequestType = req->bmRequestType;
    uint64_t wLength = req->wLength;
    uint64_t wIndex = req->wIndex;
    uint32_t dir;

    xhci_t *xhci = usb_dev_xhci(dev);
    xhci_context_t *ctx = usb_dev_xhci_context(dev);

    // Setup Stage
    addr   = (wLength << 48) | (wIndex << 32) | (wValue << 16) | (bRequest << 8) | bmRequestType;
    status = (0 << 22) | 8; // Interrupter Target = 0, TRB Transfer Length = 8
    flags  = (trt << 16) | (TRB_TYPE_SETUP_STAGE << 10) | TRB_IDT;
    xhci_command_submit(ctx->cmd, addr, status, flags);

    // Data Stage
    if(trt)
    {
        dir    = (trt == TRT_IN_DATA);
        addr   = phys;
        status = (0 << 22) | size; // Interrupter Target = 0, TRB Transfer Length = buffer size
        flags  = (dir << 16) | (TRB_TYPE_DATA_STAGE << 10);
        xhci_command_submit(ctx->cmd, addr, status, flags);
    }

    // Status Stage
    dir    = (trt > TRT_NO_DATA);
    addr   = 0;
    status = 0;
    flags  = (dir << 16) | (TRB_TYPE_STATUS_STAGE << 10) | TRB_IOC;
    xhci_command_submit(ctx->cmd, addr, status, flags);

    // Ring doorbell
    xhci_ring_doorbell(xhci, dev->slot_id, 1);
    xhci_poll_event(xhci, TRB_TYPE_TRANSFER);
}

void xhci_transfer_normal(usb_dev_t *dev, int dci, uint64_t phys, int size)
{
    xhci_context_t *ctx;
    xhci_t *xhci;
    xhci_endpoint_t *ep;
    uint32_t status, flags;
    uint64_t addr;
    int mps;

    xhci = usb_dev_xhci(dev);
    ctx = usb_dev_xhci_context(dev);

    ep = &ctx->ep[dci-1];
    mps = ep->ctx->max_packet_size;

    uint64_t start = phys;
    uint64_t curr = phys;
    uint64_t end = phys + size;

    // need to check apriori if there is enough space on the ring, otherwise we need to cycle

    // Normal TRBs
    while(curr < end)
    {
        curr++;
        if((curr % 0x10000) == 0 || curr == end)
        {
            int tdsize = (end - curr) / mps;
            if(tdsize > 31)
            {
                tdsize = 31;
            }

            addr   = start;
            status = (0 << 22) | (tdsize << 17) | (curr - start);
            flags  = (TRB_TYPE_NORMAL << 10) | TRB_CHAIN;

            if(curr < end)
            {
                flags |= TRB_ENT; // set TRB_ENT in all but the last of the Normal TRBs
            }

            xhci_command_submit(&ep->ring, addr, status, flags);
            start = curr;
        }
    }

    // Event Data TRB
    addr   = 0xdeadbeef;
    status = (0 << 22);
    flags  = (TRB_TYPE_EVENT_DATA << 10) | TRB_IOC;
    xhci_command_submit(&ep->ring, addr, status, flags);

    // Ring doorbell
    xhci_ring_doorbell(xhci, dev->slot_id, dci);
}
