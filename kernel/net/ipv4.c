#include <kernel/net/netdev.h>
#include <kernel/net/ipv4.h>

#include <kernel/debug.h>

static uint16_t ipv4_checksum(void *data, int len)
{
    uint16_t *addr = data;
    uint32_t sum = 0;

    while(len > 1)
    {
        sum += *addr++;
        len -= 2;
    }

    if(len)
    {
        sum += *(uint8_t*)addr;
    }

    while(sum > 0xFFFF)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

int ipv4_addr_add(netdev_t *dev, ipv4_addr_t *ip)
{
    list_append(&dev->ipv4, ip);
    return 0;
}

int ipv4_addr_del(netdev_t *dev, ipv4_addr_t *ip)
{
    list_remove(&dev->ipv4, ip);
    return 0;
}

void ipv4_recv(netdev_t *dev, void *payload, int size)
{
    ipv4_header_t *h = payload;

    if(ipv4_checksum(h, 4 * h->ihl))
    {
        kp_info("ipv4", "invalid checksum");
    }

    if(h->protocol == 1)
    {
        // ICMP
    }
    else if(h->protocol == 6)
    {
        // TCP
    }
    else if(h->protocol == 17)
    {
        // UDP
    }
}
