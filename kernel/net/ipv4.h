#pragma once

#include <kernel/net/netdev.h>

typedef struct {
    uint32_t address;
    uint32_t netmask;
    uint32_t prefix;
    link_t link;
} ipv4_addr_t;

typedef struct {
    netdev_t *netdev;
    uint32_t prefix;
    uint32_t netmask;
    uint32_t address;
    uint32_t nexthop;
    link_t link;
} ipv4_route_t;

int ipv4_addr_add(netdev_t *dev, ipv4_addr_t *ip);
int ipv4_addr_del(netdev_t *dev, ipv4_addr_t *ip);
