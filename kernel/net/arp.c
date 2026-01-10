#include <kernel/net/ethernet.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <kernel/mem/heap.h>
#include <kernel/time/time.h>

#include <kernel/debug.h>

static LIST_INIT(table, arp_t, link);

arp_t *arp_lookup(uint32_t tpa)
{
    arp_t *item = 0;
    while(item = list_iterate(&table, item), item)
    {
        if(item->ip == tpa)
        {
            break;
        }
    }
    return item;
}

arp_t *arp_insert(netdev_t *dev, uint32_t tpa, uint8_t *tha)
{
    arp_t *item;

    item = arp_lookup(tpa);
    if(item)
    {
        item->ts = system_timestamp();
        return item;
    }

    item = kzalloc(sizeof(*item));
    if(!item)
    {
        return 0;
    }

    item->dev = dev;
    item->ip = tpa;
    item->ts = system_timestamp();

    item->mac.addr[0] = tha[0];
    item->mac.addr[1] = tha[1];
    item->mac.addr[2] = tha[2];
    item->mac.addr[3] = tha[3];
    item->mac.addr[4] = tha[4];
    item->mac.addr[5] = tha[5];

    kp_info("arp", "ip = %d.%d.%d.%d mac = %02x:%02x:%02x:%02x:%02x:%02x",
            (tpa>>24)&0xFF,(tpa>>16)&0xFF,(tpa>>8)&0xFF,(tpa>>0)&0xFF,
            tha[0],tha[1],tha[2],tha[3],tha[4],tha[5]
        );

    list_insert(&table, item);
    return item;
}

static void arp_send_reply(netdev_t *dev, arp_packet_t *req)
{
    uint8_t *sha;
    uint8_t tmp;

    sha = dev->mac.addr;
    req->oper = ARP_REPLY;

    for(int i = 0; i < 4; i++)
    {
        tmp = req->tpa[i];
        req->tpa[i] = req->spa[i];
        req->spa[i] = tmp;
    }

    req->tha[0] = req->sha[0];
    req->tha[1] = req->sha[1];
    req->tha[2] = req->sha[2];
    req->tha[3] = req->sha[3];
    req->tha[4] = req->sha[4];
    req->tha[5] = req->sha[5];

    req->sha[0] = sha[0];
    req->sha[1] = sha[1];
    req->sha[2] = sha[2];
    req->sha[3] = sha[3];
    req->sha[4] = sha[4];
    req->sha[5] = sha[5];

    ethernet_send(dev, req->tha, 0x0806, req, sizeof(*req));
}

void arp_send_request(netdev_t *dev, uint32_t spa, uint32_t tpa)
{
    arp_packet_t req = {
        .htype = ARP_HTYPE_ETHERNET,
        .hlen  = 6,
        .ptype = ARP_PTYPE_IPV4,
        .plen  = 4,
        .oper  = ARP_REQUEST,
        .tha[0] = 0xFF,
        .tha[1] = 0xFF,
        .tha[2] = 0xFF,
        .tha[3] = 0xFF,
        .tha[4] = 0xFF,
        .tha[5] = 0xFF,
    };

    req.spa[0] = spa >> 24;
    req.spa[1] = spa >> 16;
    req.spa[2] = spa >> 8;
    req.spa[3] = spa;

    req.tpa[0] = tpa >> 24;
    req.tpa[1] = tpa >> 16;
    req.tpa[2] = tpa >> 8;
    req.tpa[3] = tpa;

    req.sha[0] = dev->mac.addr[0];
    req.sha[1] = dev->mac.addr[1];
    req.sha[2] = dev->mac.addr[2];
    req.sha[3] = dev->mac.addr[3];
    req.sha[4] = dev->mac.addr[4];
    req.sha[5] = dev->mac.addr[5];

    ethernet_send(dev, req.tha, 0x0806, &req, sizeof(req));
}

void arp_recv(netdev_t *dev, frame_t *frame)
{
    arp_packet_t *arp = frame->l3.data;
    ipv4_addr_t *item = 0;
    uint32_t spa = 0;
    uint32_t tpa = 0;

    if(arp->htype != ARP_HTYPE_ETHERNET)
    {
        return;
    }

    if(arp->ptype != ARP_PTYPE_IPV4)
    {
        return;
    }

    for(int i = 0; i < 4; i++)
    {
        spa = (spa << 8) | arp->spa[i];
        tpa = (tpa << 8) | arp->tpa[i];
    }

    if(arp->oper == ARP_REQUEST)
    {
        arp_insert(dev, spa, arp->sha);

        while(item = list_iterate(&dev->ipv4, item), item)
        {
            if(item->address == tpa)
            {
                break;
            }
        }

        if(item)
        {
            arp_send_reply(dev, arp);
        }
    }
    else if(arp->oper == ARP_REPLY)
    {
       arp_insert(dev, spa, arp->sha);
    }

    ethernet_free_frame(frame);
}
