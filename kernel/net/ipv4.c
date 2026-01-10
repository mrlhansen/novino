#include <kernel/net/ethernet.h>
#include <kernel/net/endian.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>

static LIST_INIT(routes, ipv4_route_t, link);
static LIST_INIT(fragments, ipv4_fragment_t, link);

// the ethernet_recv() function should just add frames to the queue
// a separate thread will then process frames in the queue

static ipv4_fragment_t *ipv4_fragment_cache(frame_t *frame)
{
    ipv4_fragment_t *item = 0;
    ipv4_header_t *h;
    int s;

    h = frame->l3.data;
    s = frame->l4.size;

    while(item = list_iterate(&fragments, item), item)
    {
        if(item->ident != h->ident)
        {
            continue;
        }
        if(item->saddr != h->saddr)
        {
            continue;
        }
        if(item->daddr != h->daddr)
        {
            continue;
        }
        if(item->proto != h->protocol)
        {
            continue;
        }
        break;
    }

    if(!item)
    {
        item = kzalloc(sizeof(*item));
        item->ident = h->ident;
        item->daddr = h->daddr;
        item->saddr = h->saddr;
        item->proto = h->protocol;
        item->timeout = 0; // TODO

        list_init(&item->frames, offsetof(frame_t, link));
        list_insert(&fragments, item);
    }

    list_append(&item->frames, frame);
    item->tally += s;

    if((h->flags & 1) == 0)
    {
        item->total = s + (8 * h->offset);
    }

    if(item->tally == item->total)
    {
        kp_info("ipv4", "we have all fragments!");
        return item;
    }

    return 0;
}

static void *ipv4_fragment_assemble(ipv4_fragment_t *fragment)
{
    ipv4_header_t *h;
    frame_t *item;
    void *data;

    data = kzalloc(fragment->total);
    while(item = list_pop(&fragment->frames), item)
    {
        h = item->l3.data;
        memcpy(data + 8 * h->offset, item->l4.data, item->l4.size);
        ethernet_release_frame(item);
    }

    list_remove(&fragments, fragment);
    kfree(fragment);

    return data;
}

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

void icmpv4_recv(void *payload, int size, ipv4_sdp_t *sdp)
{
    icmpv4_header_t *c;
    int s;

    c = payload;
    s = size;

    if(ipv4_checksum(c, s))
    {
        kp_info("icmp4", "invalid checksum");
        return;
    }

    if(c->type == ICMPv4_ECHO_REQUEST)
    {
        ipv4_sdp_t dsp = {
            .daddr = sdp->saddr,
            .saddr = sdp->daddr,
            .proto = sdp->proto,
        };

        c->type = ICMPv4_ECHO_REPLY;
        c->checksum = 0;
        c->checksum = ipv4_checksum(c, s);

        ipv4_send(payload, size, &dsp);
    }
}

void ipv4_send(void *payload, int size, ipv4_sdp_t *sdp)
{
    ipv4_header_t *h;
    frame_t *f;
    arp_t *arp;
    int length;
    int offset;
    int mtu;
    int s;

    arp = arp_lookup(sdp->daddr);
    if(!arp)
    {
        kp_error("ipv4", "arp lookup failed");
        return;
    }

    f = ethernet_request_frame();
    h = f->l3.data;
    s = sizeof(ipv4_header_t);

    if(!sdp->saddr)
    {
        // use default ip from dev
    }

    h->version  = 4;
    h->ihl      = 5;
    h->tos      = 0;
    h->ttl      = 64;
    h->protocol = sdp->proto;
    h->ident    = 1234; // TODO: needed a way to generate this
    h->saddr    = swap32(sdp->saddr);
    h->daddr    = swap32(sdp->daddr);

    // effective MTU
    mtu = arp->dev->mtu - s;
    mtu = mtu & -8u; // offset is in units of 8 bytes
    offset = 0;

    while(size)
    {
        length = (size > mtu) ? mtu : size;
        size = size - length;

        f->l3.size  = length + s;
        h->flags    = size ? 1 : 0;
        h->offset   = offset / 8;
        h->fragment = swap16(h->fragment);
        h->length   = swap16(f->l3.size);
        h->checksum = 0;
        h->checksum = ipv4_checksum(h, s);

        memcpy(h+1, payload + offset, length);
        ethernet_send(arp->dev, arp->mac.addr, 0x0800, f);

        offset = offset + length;
    }

    ethernet_release_frame(f);
}

void ipv4_recv(netdev_t *dev, frame_t *frame)
{
    ipv4_fragment_t *fragment;
    ipv4_header_t *h;
    void *p;
    int s;

    h = frame->l3.data;
    s = 4 * h->ihl;

    if(ipv4_checksum(h, s))
    {
        kp_info("ipv4", "invalid checksum");
        return;
    }

    h->length   = swap16(h->length);
    h->fragment = swap16(h->fragment);
    h->daddr    = swap32(h->daddr);
    h->saddr    = swap32(h->saddr);

    frame->l4.size = frame->l3.size - s;
    frame->l4.data = frame->l3.data + s;

    ipv4_sdp_t sdp = {
        .saddr = h->saddr,
        .daddr = h->daddr,
        .proto = h->protocol,
    };

    // determine if a frame is local bound or it should be forwardet
    // currently we assume eveything is local

    if((h->flags & 1) || h->offset)
    {
        fragment = ipv4_fragment_cache(frame);
        if(!fragment)
        {
            return;
        }
        // after assembly, the frame has been freed!
        s = fragment->total;
        p = ipv4_fragment_assemble(fragment);
    }
    else
    {
        fragment = 0;
        s = frame->l4.size;
        p = frame->l4.data;
    }

    if(h->protocol == 1)
    {
        // ICMP
        icmpv4_recv(p, s, &sdp);
    }
    else if(h->protocol == 6)
    {
        // TCP
    }
    else if(h->protocol == 17)
    {
        // UDP
    }

    if(fragment)
    {
        kfree(p);
        return;
    }

    ethernet_release_frame(frame);
}
