#include <kernel/net/netdev.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <string.h>

static LIST_INIT(devices, netdev_t, link);

// This file handles anything that has to do with the physical hardware
// Registering devices, changing settings like speed, mac address, etc

int netdev_register(netdev_t *dev)
{
    // check if name already exists
    list_append(&devices, dev);
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
