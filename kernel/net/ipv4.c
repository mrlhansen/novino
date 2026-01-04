#include <kernel/net/ethernet.h>
#include <kernel/net/endian.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

static LIST_INIT(routes, ipv4_route_t, link);

uint16_t ipv4_checksum(void *data, int len)
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
    // check for conflicts
    list_append(&dev->ipv4, ip);
    return 0;
}

int ipv4_addr_del(netdev_t *dev, ipv4_addr_t *ip)
{
    // check for conflicts
    list_remove(&dev->ipv4, ip);
    return 0;
}

int ipv4_route_add(netdev_t *dev, ipv4_route_t *route)
{
    // check for conflicts
	list_append(&routes, route);
	return 0;
}

ipv4_route_t *ipv4_route_find(uint32_t address)
{
	ipv4_route_t *item = 0;
    ipv4_route_t *route = 0;

    while(item = list_iterate(&routes, item), item)
    {
        if(!item->prefix) // default route
        {
            if(!route)
            {
                route = item;
            }
        }
        else if((address & item->mask) == item->subnet)
        {
            if(route)
            {
                if(item->prefix > route->prefix)
                {
                    route = item;
                }
            }
            else
            {
                route = item;
            }
        }
    }

	return route;
}

void icmpv4_recv(netdev_t *dev, void *payload)
{
    icmpv4_header_t *c;
    ipv4_header_t *h;
    int s;

    h = payload;
    c = payload + (4 * h->ihl);
    s = h->length - (4 * h->ihl);

    if(ipv4_checksum(c, s))
    {
        kp_info("icmp4", "invalid checksum");
        return;
    }

    if(c->type == ICMPv4_ECHO_REQUEST)
    {
        uint32_t tmp = h->saddr;
        h->saddr = h->daddr;
        h->daddr = tmp;

        c->type = ICMPv4_ECHO_REPLY;
        c->checksum = 0;
        c->checksum = ipv4_checksum(c, s);

        ipv4_send(dev, payload);
    }
}

void ipv4_send(netdev_t *dev, void *payload)
{
    ipv4_header_t *h;
    arp_t *a;
    int s;

    h = payload;
    s = h->length;
    a = arp_lookup(h->daddr);

    if(!a)
    {
        kp_error("ipv4", "arp lookup failed");
        return;
    }

    if(!h->ttl)
    {
        h->ttl = 64;
    }

    h->version  = 4;
    h->length   = swap16(h->length);
    h->ident    = swap16(h->ident);
    h->offset   = swap16(h->offset);
    h->daddr    = swap32(h->daddr);
    h->saddr    = swap32(h->saddr); // if saddr = 0, use default ip from dev?
    h->checksum = 0;
    h->checksum = ipv4_checksum(h, sizeof(ipv4_header_t));

    ethernet_send(a->dev, a->mac.addr, 0x0800, payload, s);
}

void ipv4_recv(netdev_t *dev, void *payload, int size) // not sure size is needed here
{
    ipv4_header_t *h = payload;

    if(ipv4_checksum(h, 4 * h->ihl))
    {
        kp_info("ipv4", "invalid checksum");
        return;
    }

    h->length = swap16(h->length);
    h->ident  = swap16(h->ident);
    h->offset = swap16(h->offset);
    h->daddr  = swap32(h->daddr);
    h->saddr  = swap32(h->saddr);

    if(h->protocol == 1)
    {
        // ICMP
        icmpv4_recv(dev, payload);
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
