#pragma once

#include <kernel/pci/pci.h>

enum {
    MAC0          = 0x00, // MAC address (6 bytes, write access must be 32-bit aligned)
    MAR0          = 0x08, // Multicast (8 bytes, write access must be 32-bit aligned)
    TxStatus0     = 0x10, // Tx status (four 32-bit registers)
    TxStartAddr0  = 0x20, // Tx start address (four 32-bit registers)
    RxBufStart    = 0x30, // Rx buffer start address
    RxEarlyCnt    = 0x30, // Early Rx byte count
    RxEarlyStatus = 0x34, // Early Rx status
    Cmd           = 0x37, // Command
    RxBufPtr      = 0x38, // Current address of packet read
    RxBufAddr     = 0x3A, // Current buffer address
    IntrMask      = 0x3C, // Interrupt mask
    IntrStatus    = 0x3E, // Interrupt status
    TxConfig      = 0x40, // Tx configuration
    RxConfig      = 0x44, // Rx configuration
    TimerCnt      = 0x48, // Timer count
    MissedCnt     = 0x4C, // Missed packet counter
    Cmd9346       = 0x50, // 93c46 command
    Config0       = 0x51, // Configuration 0
    Config1       = 0x52, // Configuration 1
    TxSummary     = 0x60, // Summary of all Tx descriptors
};

// Command
enum {
    RxBufEmpty  = 0x01, // Rx buffer empty
    CmdTxEnable = 0x04, // Tx enable
    CmdRxEnable = 0x08, // Rx enable
    CmdReset    = 0x10, // Reset
};

// Interrupt
enum {
    RxOK       = (1 << 0), // Receive OK
    RxError    = (1 << 1), // Receive error
    TxOK       = (1 << 2), // Transmit OK
    TxError    = (1 << 3), // Transmit error
    RxOverflow = (1 << 4), // Receive buffer overflow
    RxUnderrun = (1 << 5), // Receive packet underrun
    RxFIFOOver = (1 << 6), // Receive FIFO overflow
};

// RxConfig
enum {
    AcceptAll       = (1 << 0),  // Accept physical address packets
    AcceptMatch     = (1 << 1),  // Accept physical match packets
    AcceptMulticast = (1 << 2),  // Accept multicast packets
    AcceptBroadcast = (1 << 3),  // Accept broadcast packets
    AcceptRunt      = (1 << 4),  // Accept runt packets
    AcceptError     = (1 << 5),  // Accept error packets
    Wrap            = (1 << 7),  // Wrap mode
    RxDMASize64     = (2 << 8),  // Rx DMA burst size (64 bytes)
    RxDMASize128    = (3 << 8),  // Rx DMA burst size (128 bytes)
    RxDMASize256    = (4 << 8),  // Rx DMA burst size (256 bytes)
    RxBufLen8k      = (0 << 11), // Rx buffer length (8k)
    RxBufLen16k     = (1 << 11), // Rx buffer length (16k)
    RxBufLen32k     = (2 << 11), // Rx buffer length (32k)
    RxBufLen64k     = (3 << 11), // Rx buffer length (64k)
};

// Rx Packet Header
enum {
    ReceiveOK           = (1 << 0),
    FrameAlignmentError = (1 << 1),
    CRCError            = (1 << 2),
    LongPacket          = (1 << 3),
    RuntPacket          = (1 << 4),
    InvalidSymbolError  = (1 << 5),
    BroadcastPacket     = (1 << 13),
    MatchPacket         = (1 << 14),
    MulticastPacket     = (1 << 15),
};

typedef struct {
    uint32_t ioaddr;
    uint64_t hwaddr;
    uint8_t *rx_buf;
    uint32_t rx_len;
    uint32_t rx_pos;
    uint32_t tx_phys[4];
    uint8_t *tx_virt[4];
    uint32_t tx_pos;
} rtl8139_t;

void rtl8139_init(pci_dev_t *dev);
