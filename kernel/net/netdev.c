#include <kernel/net/netdev.h>
#include <kernel/net/ipv4.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <string.h>

static LIST_INIT(devices, netdev_t, link);

// This file handles anything that has to do with the physical hardware
// Registering devices, changing settings like speed, mac address, etc

int netdev_register(netdev_t *dev)
{
    // check if name already exists
    // maybe a netdev_alloc function that generates a name?

    list_append(&devices, dev);
    list_init(&dev->ipv4, offsetof(ipv4_addr_t, link));

    // tmp hack start
    static ipv4_addr_t ip = {
        .address = 0x0a0b0c02,
        .netmask = 0xffffff00,
        .prefix = 24,
    };
    list_append(&dev->ipv4, &ip);
    // tmp hack end

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
