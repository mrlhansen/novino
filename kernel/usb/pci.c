#include <kernel/usb/pci.h>
#include <kernel/usb/xhci.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>

static void uhci_pci_init(pci_dev_t *dev)
{
    pci_bar_t bar4;
    pci_read_bar(dev, 4, &bar4);
    kp_info("uhci", "UHCI: bar %lx type %x (11 = 16-bit pmio) size: %lx", bar4.value, bar4.type, bar4.size);
}

static void ohci_pci_init(pci_dev_t *dev)
{
    pci_bar_t bar0;
    pci_read_bar(dev, 0, &bar0);
    kp_info("ohci", "OHCI: bar %lx type %x size: %lx", bar0.value, bar0.type, bar0.size);
}

static void ehci_pci_init(pci_dev_t *dev)
{
    pci_bar_t bar0;
    pci_read_bar(dev, 0, &bar0);
    kp_info("ehci", "EHCI: bar %lx type %x (22 = 32-bit mmio) size: %lx", bar0.value, bar0.type, bar0.size);
}

static void xhci_pci_init(pci_dev_t *dev)
{
    uint64_t address;
    pci_bar_t bar0;

    // Read BAR
    pci_read_bar(dev, 0, &bar0);

    // Check BAR
    if((bar0.type & BAR_MMIO) == 0)
    {
        return;
    }

    // Debug
    kp_info("xhci", "mmio: %#lx, size: %#lx, gsi: %d", bar0.value, bar0.size, dev->gsi);

    // Enable bus mastering
    pci_enable_bm(dev);

    // Initialize
    address = vmm_phys_to_virt(bar0.value);
    xhci_init(dev, address);
}

void usb_init(pci_dev_t *dev)
{
    switch(dev->prog_if)
    {
        case 0x00:
            uhci_pci_init(dev);
            break;
        case 0x10:
            ohci_pci_init(dev);
            break;
        case 0x20:
            ehci_pci_init(dev);
            break;
        case 0x30:
            xhci_pci_init(dev);
            break;
    }
}
