#include <kernel/net/ethernet.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <kernel/debug.h>
#include <string.h>

void ethernet_rx_packet(netdev_t *dev, void *frame, int size)
{
    uint64_t dmac, smac;
    uint16_t type;
    uint8_t *data;

    data = frame;
    dmac = data[0];
    smac = data[6];

    for(int i = 1; i < 6; i++)
    {
        dmac = (dmac << 8) | data[i];
        smac = (smac << 8) | data[6+i];
    }

    type = data[12];
    type = (type << 8) | data[13];

    if(type == 0x8100) // 802.1q
    {
        kp_info("eth", "VLAN tagged packet, ignoring...");
        return;
    }
    else if(type == 0x0806) // ARP
    {
        arp_rx_packet(dev, frame + ETH_HLEN, size);
    }
    else if(type == 0x0800)
    {
        // ipv4
    }
}

void ethernet_tx_packet(netdev_t *dev, uint8_t *dstmac, int type, void *payload, int size) // arp entry instead of dmac?
{
    uint8_t buf[ETH_FRAME_LEN];

    buf[0] = dstmac[0];
    buf[1] = dstmac[1];
    buf[2] = dstmac[2];
    buf[3] = dstmac[3];
    buf[4] = dstmac[4];
    buf[5] = dstmac[5];

    buf[6] = dev->mac >> 40;
    buf[7] = dev->mac >> 32;
    buf[8] = dev->mac >> 24;
    buf[9] = dev->mac >> 16;
    buf[10] = dev->mac >> 8;
    buf[11] = dev->mac;

    buf[12] = (type >> 8);
    buf[13] = type;

    memcpy(buf + ETH_HLEN, payload, size);
    dev->ops->transmit(dev, buf, size + ETH_HLEN);
}
