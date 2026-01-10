#include <kernel/sched/kthreads.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/endian.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>
#include <string.h>

static LIST_INIT(frames, frame_t, link);
static LIST_INIT(queue, frame_t, link);
static thread_t *worker;

// TODO: Per device? since MTU can be different
// We can also make this a function of the device? dev->request_frame
frame_t *ethernet_request_frame()
{
    frame_t *frame;

    frame = list_pop(&frames);
    if(frame)
    {
        return frame;
    }

    frame = kzalloc(sizeof(frame_t) + 1516);
    if(!frame)
    {
        return 0; // TODO: this is panic-level bad
    }

    frame->l2.data = (void*)(frame+1);
    frame->l3.data = frame->l2.data + ETH_HLEN;

    return frame;
}

void ethernet_release_frame(frame_t *frame)
{
    list_append(&frames, frame);
}

void ethernet_recv(netdev_t *dev, void *data, int size)
{
    mac_header_t *mhdr;
    frame_t *frame;

    mhdr = data;
    frame = ethernet_request_frame();
    memcpy(frame->l2.data, data, size);

    frame->dev = dev;
    frame->type = swap16(mhdr->type);
    frame->l2.size = size;
    frame->l3.size = size - ETH_HLEN - ETH_FCS_LEN;

    list_append(&queue, frame);

    if(worker->state == BLOCKING)
    {
        thread_unblock(worker);
    }
}

void ethernet_send(netdev_t *dev, uint8_t *dmac, int type, frame_t *frame) // frame contains type and dev!
{
    mac_header_t *mhdr;
    uint8_t *smac;

    smac = dev->mac.addr;
    mhdr = frame->l2.data;
    mhdr->type = swap16(type);

    mhdr->dmac[0] = dmac[0];
    mhdr->dmac[1] = dmac[1];
    mhdr->dmac[2] = dmac[2];
    mhdr->dmac[3] = dmac[3];
    mhdr->dmac[4] = dmac[4];
    mhdr->dmac[5] = dmac[5];

    mhdr->smac[0] = smac[0];
    mhdr->smac[1] = smac[1];
    mhdr->smac[2] = smac[2];
    mhdr->smac[3] = smac[3];
    mhdr->smac[4] = smac[4];
    mhdr->smac[5] = smac[5];

    dev->ops->transmit(dev, mhdr, frame->l3.size + ETH_HLEN);
}

static void ethernet_worker()
{
    frame_t *frame;

    while(1)
    {
        frame = list_pop(&queue);
        if(!frame)
        {
            thread_block(0);
            continue;
        }

        switch(frame->type)
        {
            case 0x0806:
                arp_recv(frame->dev, frame);
                break;
            case 0x0800:
                ipv4_recv(frame->dev, frame);
                break;
            default:
                kp_info("net", "unknown ethertype %x", frame->type);
                ethernet_release_frame(frame);
                break;
        }
    }
}

void ethernet_init()
{
    worker = kthreads_create("net", ethernet_worker, 0, TPR_SRT);
    kthreads_run(worker);
}
