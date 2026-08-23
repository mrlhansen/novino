#define RTL8169_MODULE

#include <kernel/net/ethernet.h>
#include <kernel/net/rtl8169.h>
#include <kernel/x86/ioports.h>
#include <kernel/x86/irq.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/mmio.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>

static int rtl8169_status(netdev_t *dev)
{
    rtl8169_t *rtl;
    int status;

    rtl = dev->data;
    status = inportb(rtl->ioaddr + PhyStatus);

    if(status & PhyLink)
    {
        dev->flags |= NetdevLinkUp;

        if(status & Phy1000M)
        {
            dev->speed = 1000;
        }
        else if(status & Phy100M)
        {
            dev->speed = 100;
        }
        else if(status & Phy10M)
        {
            dev->speed = 10;
        }

        if(status & PhyDuplex)
        {
            dev->flags |= NetdevFullDuplex;
        }
    }

    return 0;
}

static int rtl8169_transmit(netdev_t *dev, void *data, int size)
{
    rtl8169_descriptor_t *tx;
    rtl8169_t *rtl;
    void *buf;
    int offset;

    rtl = dev->data;
    tx = rtl->tx + rtl->tx_pos;
    // we should technically check that OWN=0
    offset = tx->addr - rtl->tx_buf_phys;
    buf = (void*)(rtl->tx_buf_virt + offset);

    memcpy(buf, data, size);
    if(size < ETH_ZLEN)
    {
        size = ETH_ZLEN;
    }

    tx->command = (FS | LS | size);
    tx->vlan = 0;

    // cycle
    rtl->tx_pos++;
    if(rtl->tx_pos == rtl->tx_len)
    {
        rtl->tx_pos = 0;
        tx->command |= EOR;
    }

    // send
    tx->command |= OWN;
    outportb(rtl->ioaddr + TxPoll, 0x40);

    return 0;
}

static void rtl8169_receive(rtl8169_t *rtl)
{
    rtl8169_descriptor_t *rx;
    void *buf;
    int length;
    int offset;

    rx = rtl->rx + rtl->rx_pos;

    while((rx->command & OWN) == 0)
    {
        length = rx->command & 0x3FFF;
        offset = rx->addr - rtl->rx_buf_phys;

        buf = (void*)(rtl->rx_buf_virt + offset);
        // check error flags?
        ethernet_recv(rtl->netdev, buf, length);

        // cycle
        rx->command = (OWN | MaxRxSize);
        rtl->rx_pos++;

        if(rtl->rx_pos == rtl->rx_len)
        {
            rtl->rx_pos = 0;
            rx->command |= EOR;
            rx = rtl->rx;
        }
        else
        {
            rx++;
        }
    }
}

static void rtl8169_handler(int vector, void *data)
{
    uint16_t status;
    rtl8169_t *rtl;

    rtl = data,
    status = inportw(rtl->ioaddr + IntrStatus);
    outportw(rtl->ioaddr + IntrStatus, status);

    if(status & RxOK)
    {
        rtl8169_receive(rtl);
    }

    if(status & LinkChange)
    {
        netdev_check_status(rtl->netdev);
    }
}

static int rtl8169_init_descriptors(rtl8169_t *rtl)
{
    uint64_t virt, phys;
    uint32_t RxMemSize;
    uint64_t RxAddress;
    uint32_t TxMemSize;
    uint64_t TxAddress;

    RxMemSize = NumRxDesc * (sizeof(rtl8169_descriptor_t) + MaxRxSize);
    TxMemSize = NumTxDesc * (sizeof(rtl8169_descriptor_t) + MaxTxSize);

    if(mmio_alloc_uc_region(RxMemSize + TxMemSize, 256, &virt, &phys))
    {
        return -ENOMEM;
    }

    memset((void*)virt, 0, RxMemSize + TxMemSize); // maybe this should be done automatically

    rtl->rx = (void*)virt;
    rtl->rx_phys = phys;
    rtl->tx = (void*)(virt + RxMemSize);
    rtl->tx_phys = phys + RxMemSize;

    RxAddress = rtl->rx_phys + (NumRxDesc * sizeof(rtl8169_descriptor_t));
    TxAddress = rtl->tx_phys + (NumRxDesc * sizeof(rtl8169_descriptor_t));

    rtl->rx_buf_phys = RxAddress;
    rtl->rx_buf_virt = (RxAddress - phys) + virt;
    rtl->tx_buf_phys = TxAddress;
    rtl->tx_buf_virt = (TxAddress - phys) + virt;

    for(int i = 0; i < NumRxDesc; i++)
    {
        rtl->rx[i].command = OWN | MaxRxSize;
        rtl->rx[i].addr = RxAddress;
        RxAddress += MaxRxSize;
    }

    // set EOR bit
    rtl->rx[NumRxDesc - 1].command |= EOR;

    for(int i = 0; i < NumTxDesc; i++)
    {
        rtl->tx[i].addr = TxAddress;
        TxAddress += MaxTxSize;
    }

    return 0;
}

static void rtl8169_init_device(pci_dev_t *pcidev, uint32_t ioaddr)
{
    rtl8169_t *rtl;
    uint32_t bits;
    hwaddr_t mac;
    int status;
    int vector;

    // Read MAC address
    for(int i = 0; i < 6; i++)
    {
        mac.addr[i] = inportb(ioaddr + MAC0 + i);
    }

    // Allocate data
    rtl = kzalloc(sizeof(rtl8169_t));
    rtl->ioaddr = ioaddr;
    rtl->hwaddr = mac;

    // Reset chip
    outportb(ioaddr + Cmd, CmdReset);
    for(int i = 0; i < 1000; i++)
    {
        status = inportb(ioaddr + Cmd);
        if(status & CmdReset)
        {
            break;
        }
    }

    // Setup descriptors
    rtl8169_init_descriptors(rtl);

    // Unlock config registers
    outportb(ioaddr + Cfg9346, 0xC0);

    // RxConfig = RXFTH: unlimited, MXDMA: unlimited, AB: set, APM: set
    outportl(ioaddr + RxConfig, 0x0000E70A);

    // Enable transmit, required before setting TxConfig
    outportb(ioaddr + Cmd, CmdTxEnable);

    // TxConfig = IFG: normal, MXDMA: unlimited
    outportl(ioaddr + TxConfig, 0x03000700);

    // Max receive packet size
    outportw(ioaddr + RxMS, MaxRxSize);

    // Max transmit packet size
    outportb(ioaddr + TxMS, MaxTxSize / 128);

    // Set address of Tx descriptors
    outportl(ioaddr + TxDescAddrLow, rtl->tx_phys);

    // Set address of Rx descriptors
    outportl(ioaddr + RxDescAddrLow, rtl->rx_phys);

    // IRQ handler
    status = pci_alloc_irq_vectors(pcidev, 1, 1);
    if(status < 0)
    {
        kp_error("r8169", "failed to allocate IRQ vectors (status %d)", status);
        return;
    }

    vector = pci_irq_vector(pcidev, 0);
    irq_request(vector, rtl8169_handler, rtl);

    // Set interrupt bits
    bits = (RxOK | TxOK | LinkChange);
    outportw(ioaddr + IntrMask, bits);

    // Enable receive and transmit
    bits = (CmdTxEnable | CmdRxEnable);
    outportb(ioaddr + Cmd, bits);

    // Lock config registers
    outportb(ioaddr + Cfg9346, 0x00);

    // Logging
    kp_info("r8169", "iobase: %#x, hwaddr: %02x:%02x:%02x:%02x:%02x:%02x", ioaddr, mac.addr[0], mac.addr[1], mac.addr[2], mac.addr[3], mac.addr[4], mac.addr[5]);

    // Register device
    static netdev_ops_t ops = {
        .transmit = rtl8169_transmit,
        .status = rtl8169_status,
    };

    netdev_t *netdev = kzalloc(sizeof(*netdev));
    rtl->netdev = netdev;

    strcpy(netdev->name, "eth0");
    netdev->flags = NetdevEthernet;
    netdev->mac = mac;
    netdev->mtu = 1500;
    netdev->data = rtl;
    netdev->ops = &ops;

    netdev_register(netdev);
}

void rtl8169_init(pci_dev_t *dev)
{
    pci_bar_t bar0;

    // Check GSI value
    if(dev->gsi < 0)
    {
        kp_error("r8169", "invalid GSI value");
        return;
    }

    // Read BAR0
    pci_read_bar(dev, 0, &bar0);

    // Check BAR
    if((bar0.type & BAR_PMIO) == 0)
    {
        return;
    }

    // Enable bus mastering
    pci_enable_bm(dev);

    // Initialize device
    rtl8169_init_device(dev, bar0.value);
}
