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

typedef struct {
	uint8_t  ihl     : 4;  // Internet header length (in units of 4 bytes)
	uint8_t  version : 4;  // Version (always 4)
	uint8_t  tos;          // Type of service
	uint16_t length;       // Total length (big-endian)
	uint16_t ident;        // Identification (big-endian)
	uint16_t offset;       // Flags and fragment offset (big-endian)
	uint8_t  ttl;          // Time to live
	uint8_t  protocol;     // Protocol
	uint16_t checksum;     // Header checksum
	uint32_t saddr;        // Source address (big-endian)
	uint32_t daddr;        // Destination address (big-endian)
} __attribute__((packed)) ipv4_header_t;

int ipv4_addr_add(netdev_t *dev, ipv4_addr_t *ip);
int ipv4_addr_del(netdev_t *dev, ipv4_addr_t *ip);

void ipv4_recv(netdev_t *dev, void *payload, int size);
