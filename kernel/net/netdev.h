#pragma once

#include <kernel/lists.h>

#define NETDEV_LOOPBACK 0x01
#define NETDEV_ETHERNET 0x02

typedef struct netdev netdev_t;

typedef struct {
    int (*transmit)(netdev_t*,void*,int);
} netdev_ops_t;

struct netdev {
    char name[16];
    uint32_t flags;
    uint64_t hwaddr;
    netdev_ops_t *ops;
    void *data;
    link_t link;
};

int netdev_register(netdev_t *dev);
netdev_t *netdev_lookup(const char *name);
