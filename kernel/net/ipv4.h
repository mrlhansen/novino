#pragma once

#include <kernel/net/netdev.h>

typedef struct {
    uint32_t address; // Address
    uint32_t netmask; // Subnet mask
    uint32_t prefix;  // Subnet prefix
    link_t link;      // Link in list of addresses
} ipv4_addr_t;

typedef struct {
    netdev_t *dev;    // Network device
    uint32_t subnet;  // Subnet address
    uint32_t prefix;  // Subnet prefix
    uint32_t mask;    // Subnet mask
    uint32_t nexthop; // Next hop address
    uint32_t source;  // Source address (defaults to first address of device)
    link_t link;      // Link in list of routes
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

enum {
    ICMPv4_ECHO_REPLY        = 0,
    ICMPv4_UNREACHABLE       = 3,
    ICMPv4_ECHO_REQUEST      = 8,
    ICMPv4_TIME_EXCEEDED     = 11,
    ICMPv4_PARAMETER_PROBLEM = 12,
};

typedef struct {
    uint8_t  type;      // Type
    uint8_t  code;      // Subtype
    uint16_t checksum;  // Checksum (header and data)
    uint32_t rest;      // Rest of header, depending on type
} __attribute__((packed)) icmpv4_header_t;

int ipv4_route_add(netdev_t *dev, ipv4_route_t *route);
ipv4_route_t *ipv4_route_find(uint32_t address);

int ipv4_addr_add(netdev_t *dev, ipv4_addr_t *ip);
int ipv4_addr_del(netdev_t *dev, ipv4_addr_t *ip);

uint16_t ipv4_checksum(void *data, int len);

void ipv4_send(netdev_t *dev, void *payload);
void ipv4_recv(netdev_t *dev, void *payload, int size);
