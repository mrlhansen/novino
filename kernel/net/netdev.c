#include <kernel/net/netdev.h>
#include <kernel/net/ipv4.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

#include <kernel/net/arp.h>

static LIST_INIT(devices, netdev_t, link);

// This file handles anything that has to do with the physical hardware
// Registering devices, changing settings like speed, mac address, etc

void netdev_check_status(netdev_t *dev)
{
    char *duplex;

    // reset status
    dev->flags &= ~(NetdevLinkUp | NetdevFullDuplex);
    dev->speed = 0;

    // update status
    dev->ops->status(dev);

    // logging
    if(dev->flags & NetdevLinkUp)
    {
        if(dev->flags & NetdevFullDuplex)
        {
            duplex = "full-duplex";
        }
        else
        {
            duplex = "half-duplex";
        }
        kp_info("net", "%s: link up, %d mbps, %s", dev->name, dev->speed, duplex);
    }
    else
    {
        kp_info("net", "%s: link down", dev->name);
    }
}

int netdev_register(netdev_t *dev)
{
    // check if name already exists
    // maybe a netdev_alloc function that generates a name?

    list_append(&devices, dev);
    list_init(&dev->ipv4, offsetof(ipv4_addr_t, link));

    // tmp hack start
    const uint32_t myip = (192u << 24) | (168u << 16) | (0u << 8) | (123u << 0);
    static ipv4_addr_t ip = {
        .address = myip,
        .netmask = 0xffffff00,
        .prefix = 24,
    };
    list_append(&dev->ipv4, &ip);
    static ipv4_route_t rt = {
        .subnet = myip & 0xffffff00,
        .prefix = 24,
        .mask = 0xffffff00,
        .nexthop = 0,
        .source = myip,
    };
    rt.dev = dev;
    ipv4_add_route(&rt);
    // tmp hack end

    netdev_check_status(dev);

    return 0;
}

netdev_t *netdev_lookup(const char *name)
{
    netdev_t *item = 0;
    while(item = list_iterate(&devices, item), item)
    {
        if(strcmp(item->name, name) == 0)
        {
            break;
        }
    }
    return item;
}
