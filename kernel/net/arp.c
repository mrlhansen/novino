#include <kernel/net/ethernet.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>

#include <kernel/debug.h>

void arp_send_reply(netdev_t *dev, arp_packet_t *req) // make this function more generic, e.g. take type as input
{
    ipv4_addr_t *ip = dev->ipv4.head;

    req->oper = 0x0200;

    req->tpa[0] = req->spa[0];
    req->tpa[1] = req->spa[1];
    req->tpa[2] = req->spa[2];
    req->tpa[3] = req->spa[3];

    req->spa[0] = ip->address >> 24;
    req->spa[1] = ip->address >> 16;
    req->spa[2] = ip->address >> 8;
    req->spa[3] = ip->address;

    req->tha[0] = req->sha[0];
    req->tha[1] = req->sha[1];
    req->tha[2] = req->sha[2];
    req->tha[3] = req->sha[3];
    req->tha[4] = req->sha[4];
    req->tha[5] = req->sha[5];

    req->sha[0] = dev->mac >> 40;
    req->sha[1] = dev->mac >> 32;
    req->sha[2] = dev->mac >> 24;
    req->sha[3] = dev->mac >> 16;
    req->sha[4] = dev->mac >> 8;
    req->sha[5] = dev->mac;

    ethernet_tx_packet(dev, req->tha, 0x0806, req, sizeof(*req));
}

void arp_rx_packet(netdev_t *dev, void *frame, int size)
{
    arp_packet_t *arp;
    arp = frame;

    if(arp->htype != 0x0100)
    {
        kp_info("arp", "unknown htype (%d)", arp->htype);
        return;
    }

    if(arp->ptype != 0x0008)
    {
        kp_info("arp", "unknown ptype (%d)", arp->ptype);
        return;
    }

    kp_info("arp","ARP request for %d.%d.%d.%d from %d.%d.%d.%d (%02x:%02x:%02x:%02x:%02x:%02x)",
            arp->tpa[0],arp->tpa[1],arp->tpa[2],arp->tpa[3],
            arp->spa[0],arp->spa[1],arp->spa[2],arp->spa[3],
            arp->sha[0],arp->sha[1],arp->sha[2],arp->sha[3],arp->sha[4],arp->sha[5]
        );

    if(arp->oper == 0x0100) // request
    {
        // check if we have the IP address
        arp_send_reply(dev, arp);
    }
    else if(arp->oper == 0x0200) // reply
    {

    }
}
