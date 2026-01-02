#include <kernel/net/netdev.h>
#include <kernel/net/ipv4.h>

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
