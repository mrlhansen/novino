#pragma once

#include <kernel/net/netdev.h>

#define ETH_ALEN       6     // Octets in one ethernet address
#define ETH_HLEN       14    // Total octets in header
#define ETH_ZLEN       60    // Minimum octets in frame sans FCS
#define ETH_DATA_LEN   1500  // Maximum octets in payload
#define ETH_FRAME_LEN  1514  // Maximum octets in frame sans FCS
#define ETH_FCS_LEN    4     // Octets in the FCS (Frame Check Sequence)

typedef struct {
    int size;
    void *data;
} layer_t;

typedef struct {
    int type;
    layer_t l2;
    layer_t l3;
    layer_t l4;
    netdev_t *dev;
    link_t link;
} frame_t;

typedef struct {
    uint8_t dmac[6];
    uint8_t smac[6];
    uint16_t type;
} mac_header_t;

frame_t *ethernet_request_frame();
void ethernet_release_frame(frame_t *frame);

void ethernet_recv(netdev_t *dev, void *data, int size);
void ethernet_send(netdev_t *dev, uint8_t *dmac, int type, frame_t *frame);

void ethernet_init();
