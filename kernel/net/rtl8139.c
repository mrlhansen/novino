#include <kernel/net/ethernet.h>
#include <kernel/net/rtl8139.h>
#include <kernel/x86/ioports.h>
#include <kernel/x86/irq.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/mmio.h>
#include <kernel/debug.h>
#include <string.h>

static int rtl8139_transmit(netdev_t *dev, void *data, int size)
{
    int offset, next;
    rtl8139_t *rtl;
    uint32_t value;

    rtl = dev->data;
    next = rtl->tx_pos;
    rtl->tx_pos = (next + 1) % 4;

    memcpy(rtl->tx_virt[next], data, size);
    if(size < ETH_ZLEN)
    {
        size = ETH_ZLEN;
    }

    offset = TxStartAddr0 + (4 * next);
    value = rtl->tx_phys[next];
    outportl(rtl->ioaddr + offset, value);

    offset = TxStatus0 + (4 * next);
    outportl(rtl->ioaddr + offset, size);

    return 0;
}

static void rtl8139_receive(rtl8139_t *rtl)
{
    uint8_t *buf;
    int flags;
    int length;

    while((inportb(rtl->ioaddr + Cmd) & RxBufEmpty) == 0)
    {
        // First 2 bytes is the packet flags
        // Next 2 bytes is the packet length
        buf = rtl->rx_buf + rtl->rx_pos;
        flags = *(uint16_t*)buf;
        length = *(uint16_t*)(buf + 2);

        if(flags & ReceiveOK)
        {
            ethernet_rx_packet(rtl->netdev, buf + 4, length);
        }

        rtl->rx_pos = ((rtl->rx_pos + length + 7) & -4) % rtl->rx_len;
        outportw(rtl->ioaddr + RxBufPtr, rtl->rx_pos - 16);
    }
}

static void rtl8139_handler(int vector, void *data)
{
    uint16_t summary;
    uint16_t status;
    rtl8139_t *rtl;

    rtl = data,
    status = inportw(rtl->ioaddr + IntrStatus);
    outportw(rtl->ioaddr + IntrStatus, status);

    if(status & RxOK)
    {
        rtl8139_receive(rtl);
    }

    if(status & TxOK)
    {
        summary = inportw(rtl->ioaddr + TxSummary);
        kp_info("r8139", "packet transmitted, summary %x", summary);
    }
}

static void rtl8139_init_device(pci_dev_t *pcidev, uint32_t ioaddr)
{
    rtl8139_t *rtl;
    uint64_t virt, phys;
    uint64_t mac = 0;
    uint32_t bits;
    int status;
    int vector;

    // Read MAC address
    for(int i = 0; i < 6; i++)
    {
        mac = (mac << 8) | inportb(ioaddr + MAC0 + i);
    }

    // Power on
    outportb(ioaddr + Config1, 0x0);

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

    // Allocate memory
    // 8192 + 16 + 1500 ~= 3*4096 for receive buffer
    // 4*2048 = 2*4096 for transmit buffer
    if(mmio_alloc_uc_region(5*4096, 4, &virt, &phys))
    {
        return;
    }
    memset((void*)virt, 0, 5*4096);

    // Set receive buffer
    outportl(ioaddr + RxBufStart, phys);

    // Set interrupt bits
    bits = (RxOK | TxOK);
    outportw(ioaddr + IntrMask, bits);

    // Configure Rx
    bits = (AcceptMatch | AcceptBroadcast | AcceptMulticast | Wrap | RxBufLen8k);
    outportl(ioaddr + RxConfig, bits);

    // Save information
    rtl = kzalloc(sizeof(rtl8139_t));
    rtl->ioaddr = ioaddr;
    rtl->hwaddr = mac;
    rtl->rx_buf = (void*)virt;
    rtl->rx_len = 8192;
    rtl->rx_pos = 0;
    rtl->tx_pos = 0;

    for(int i = 0; i < 4; i++)
    {
        rtl->tx_virt[i] = (void*)(virt + 3*4096 + i*2048);
        rtl->tx_phys[i] = (phys + 3*4096 + i*2048);
    }

    // IRQ handler
    status = pci_alloc_irq_vectors(pcidev, 1, 1);
    if(status < 0)
    {
        kp_error("r8139", "failed to allocate IRQ vectors (status %d)", status);
        return;
    }

    vector = pci_irq_vector(pcidev, 0);
    irq_request(vector, rtl8139_handler, rtl);

    // Enable receive and transmit
    bits = (CmdTxEnable | CmdRxEnable);
    outportb(ioaddr + Cmd, bits);

    // Logging
    kp_info("r8139", "iobase: %#x, mac: %lx", ioaddr, mac);

    // Register device
    static netdev_ops_t ops = {
        .transmit = rtl8139_transmit,
    };

    netdev_t *netdev = kzalloc(sizeof(*netdev));
    rtl->netdev = netdev;

    strcpy(netdev->name, "eth0");
    netdev->flags = NETDEV_ETHERNET;
    netdev->mac = mac;
    netdev->mtu = 1500;
    netdev->data = rtl;
    netdev->ops = &ops;

    netdev_register(netdev);
}

void rtl8139_init(pci_dev_t *dev)
{
    pci_bar_t bar0;

    // Check GSI value
    if(dev->gsi < 0)
    {
        kp_error("r8139", "invalid GSI value");
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
    rtl8139_init_device(dev, bar0.value);
}
