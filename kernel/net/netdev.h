#pragma once

#include <kernel/lists.h>

enum {
    NetdevLoopback   = (1u << 0),
    NetdevEthernet   = (1u << 1),
    NetdevLinkUp     = (1u << 2),
    NetdevFullDuplex = (1u << 3),
};

typedef struct netdev netdev_t;

typedef struct {
    uint8_t addr[6];
} hwaddr_t;

typedef struct {
    int (*transmit)(netdev_t*,void*,int);
    int (*status)(netdev_t*);
} netdev_ops_t;

struct netdev {
    char name[16];
    uint32_t flags;
    uint32_t speed;
    uint32_t mtu;
    hwaddr_t mac;
    netdev_ops_t *ops;
    void *data;
    link_t link;
    list_t ipv4;
};

int netdev_register(netdev_t *dev);
void netdev_check_status(netdev_t *dev);
netdev_t *netdev_lookup(const char *name);
