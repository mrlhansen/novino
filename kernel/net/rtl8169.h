#pragma once

#include <kernel/pci/pci.h>
#include <kernel/net/netdev.h>

#ifdef RTL8169_MODULE

// Configuration
enum {
    MaxRxSize = 1536, // Maximum Rx packet size
    MaxTxSize = 1536, // Maximum Tx packet size (must be divisible by 128)
    NumRxDesc = 128,  // Number of Rx descriptors
    NumTxDesc = 128,  // Number of Tx descriptors
};

// Registers
enum {
    MAC0                   = 0x00, // MAC address (6 bytes, write access must be 32-bit aligned)
    MAR0                   = 0x08, // Multicast (8 bytes, write access must be 32-bit aligned)
    TxDescAddrLow          = 0x20, // Tx descriptor low address (normal priority)
    TxDescAddrHigh         = 0x24, // Tx descriptor high address (normal priority)
    TxHighPrioDescAddrLow  = 0x28, // Tx descriptor low address (high priority)
    TxHighPrioDescAddrHigh = 0x2C, // Tx descriptor high address (high priority)
    Cmd                    = 0x37, // Command
    TxPoll                 = 0x38, // Tx polling
    IntrMask               = 0x3C, // Interrupt mask
    IntrStatus             = 0x3E, // Interrupt status
    TxConfig               = 0x40, // Tx configuration
    RxConfig               = 0x44, // Rx configuration
    Cfg9346                = 0x50, // Configuration
    PhyStatus              = 0x6C, // Physical status
    RxMS                   = 0xDA, // Rx maximum packet size
    RxDescAddrLow          = 0xE4, // Rx descriptor low address
    RxDescAddrHigh         = 0xE8, // Rx descriptor high address
    TxMS                   = 0xEC, // Tx maximum packet size (in units of 128 bytes)
};

// Commands
enum {
    CmdTxEnable = 0x04, // Tx enable
    CmdRxEnable = 0x08, // Rx enable
    CmdReset    = 0x10, // Reset
};

// Interrupt status
enum {
    RxOK             = (1 << 0), // Receive OK
    RxError          = (1 << 1), // Receive error
    TxOK             = (1 << 2), // Transmit OK
    TxError          = (1 << 3), // Transmit error
    RxDescUnavilable = (1 << 4), // Receive descriptor unavailable
    LinkChange       = (1 << 5), // Link status changed
    RxFifoOverflow   = (1 << 6), // Receive FIFO overflow
    TxDescUnavilable = (1 << 7), // Receive descriptor unavailable
};

// Physical status
enum {
    PhyTxFlow = 0x40, // Transmit flow control
    PhyRxFlow = 0x20, // Receive flow control
    Phy1000M  = 0x10, // Link speed is 1000 Mbps
    Phy100M   = 0x08, // Link speed is 100 Mbps
    Phy10M    = 0x04, // Link speed is 10 Mbps
    PhyLink   = 0x02, // Link status (up or down)
    PhyDuplex = 0x01, // Full-duplex (1) or half-duplex (0)
};

// Descriptor status
enum {
    OWN = (1U << 31),
    EOR = (1U << 30),
    FS  = (1U << 29),
    LS  = (1U << 28),
};

typedef struct {
    uint32_t command;
    uint32_t vlan;
    uint64_t addr; // Physical buffer address
} rtl8169_descriptor_t;

typedef struct {
    uint32_t ioaddr;
    hwaddr_t hwaddr;

    rtl8169_descriptor_t *rx;
    uint32_t rx_len;
    uint32_t rx_pos;
    uintptr_t rx_phys;
    uintptr_t rx_buf_phys;
    uintptr_t rx_buf_virt;

    rtl8169_descriptor_t *tx;
    uint32_t tx_len;
    uint32_t tx_pos;
    uintptr_t tx_phys;
    uintptr_t tx_buf_phys;
    uintptr_t tx_buf_virt;

    netdev_t *netdev;
} rtl8169_t;

#endif

void rtl8169_init(pci_dev_t *dev);
