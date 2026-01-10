#include <kernel/net/ethernet.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>
#include <string.h>

static LIST_INIT(frames, frame_t, link);

frame_t *ethernet_alloc_frame() // TODO: Per device? since MTU can be different
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

void ethernet_free_frame(frame_t *frame)
{
    list_append(&frames, frame);
}

void ethernet_recv(netdev_t *dev, void *dmaptr, int size)
{
    uint16_t type;
    uint8_t *data;
    frame_t *frame;

    data = dmaptr;
    type = data[12];
    type = (type << 8) | data[13];

    frame = ethernet_alloc_frame();
    memcpy(frame->l2.data, dmaptr, size);
    frame->l2.size = size;
    frame->l3.size = size - ETH_HLEN - ETH_FCS_LEN;

    if(type == 0x8100)
    {
        // 802.1q tagged
        return;
    }
    else if(type == 0x0806)
    {
        arp_recv(dev, frame);
    }
    else if(type == 0x0800)
    {
        ipv4_recv(dev, frame);
    }
    else if(type == 0x86DD)
    {
        // IPv6
        return;
    }
}

static uint8_t buf[ETH_FRAME_LEN]; // make a buffer system for packets

void ethernet_send(netdev_t *dev, uint8_t *addr, int type, void *payload, int size)
{
    uint8_t *smac, *dmac;

    smac = dev->mac.addr;
    dmac = addr;

    buf[0]  = dmac[0];
    buf[1]  = dmac[1];
    buf[2]  = dmac[2];
    buf[3]  = dmac[3];
    buf[4]  = dmac[4];
    buf[5]  = dmac[5];

    buf[6]  = smac[0];
    buf[7]  = smac[1];
    buf[8]  = smac[2];
    buf[9]  = smac[3];
    buf[10] = smac[4];
    buf[11] = smac[5];

    buf[12] = type >> 8;
    buf[13] = type;

    memcpy(buf + ETH_HLEN, payload, size);
    dev->ops->transmit(dev, buf, size + ETH_HLEN);
}
