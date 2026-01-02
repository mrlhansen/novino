#pragma once

#include <kernel/net/netdev.h>

typedef struct {
    uint16_t htype;  // Hardware type
    uint16_t ptype;  // Protocol type
    uint8_t hlen;    // Hardware length
    uint8_t plen;    // Protocol length
    uint16_t oper;   // Operation
    uint8_t sha[6];  // Sender hardware address
    uint8_t spa[4];  // Sender protocol address
    uint8_t tha[6];  // Target hardware address
    uint8_t tpa[4];  // Target protocol address
} __attribute__((packed)) arp_packet_t;

void arp_rx_packet(netdev_t *dev, void *frame, int size);
