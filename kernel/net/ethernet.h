#pragma once

#include <kernel/net/netdev.h>

#define ETH_ALEN       6     // Octets in one ethernet address
#define ETH_HLEN       14    // Total octets in header
#define ETH_ZLEN       60    // Minimum octets in frame sans FCS
#define ETH_DATA_LEN   1500  // Maximum octets in payload
#define ETH_FRAME_LEN  1514  // Maximum octets in frame sans FCS
#define ETH_FCS_LEN    4     // Octets in the FCS (Frame Check Sequence)

void ethernet_rx_packet(netdev_t *dev, void *frame, int size);
void ethernet_tx_packet(netdev_t *dev, uint8_t *dstmac, int type, void *payload, int size);
