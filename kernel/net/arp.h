#pragma once

#include <kernel/net/netdev.h>

enum {
    ARP_HTYPE_ETHERNET = 0x0100, // ARP ethernet hardware (big-endian)
    ARP_PTYPE_IPV4     = 0x0008, // ARP IPv4 protocol (big-endian)
};

enum {
    ARP_REQUEST = 0x0100, // ARP request (big-endian)
    ARP_REPLY   = 0x0200, // ARP reply (big-endian)
};

typedef struct {
    uint16_t htype;  // Hardware type (big-endian)
    uint16_t ptype;  // Protocol type (big-endian)
    uint8_t hlen;    // Hardware length
    uint8_t plen;    // Protocol length
    uint16_t oper;   // Operation (big-endian)
    uint8_t sha[6];  // Sender hardware address
    uint8_t spa[4];  // Sender protocol address
    uint8_t tha[6];  // Target hardware address
    uint8_t tpa[4];  // Target protocol address
} __attribute__((packed)) arp_packet_t;

typedef struct {
    uint32_t ip;   // IPv4 address
    hwaddr_t mac;  // MAC address
    uint64_t ts;   // Timestamp for latest refresh
    netdev_t *dev; // Associated network device
    link_t link;
} arp_t;

arp_t *arp_lookup(uint32_t tpa);
arp_t *arp_insert(netdev_t *dev, uint32_t tpa, uint8_t *tha);

void arp_send_request(netdev_t *dev, uint32_t spa, uint32_t tpa);
void arp_recv(netdev_t *dev, void *payload, int size);
