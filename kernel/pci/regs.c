#include <kernel/pci/pci.h>
#include <kernel/x86/ioports.h>

void pci_read_byte(pci_dev_t *dev, int offset, uint8_t *value)
{
    uint32_t reg;

    if(dev->mmio)
    {
        reg = (offset & 0xFFF);
        *value = *(volatile uint8_t*)(dev->mmio + reg);
        return;
    }

    reg = (0x80000000 | (dev->pmio << 8) | (offset & 0xFC));
    outportl(CONFIG_ADDR, reg);
    *value = inportb(CONFIG_DATA + (offset & 3));
}

void pci_read_word(pci_dev_t *dev, int offset, uint16_t *value)
{
    uint32_t reg;

    if(dev->mmio)
    {
        reg = (offset & 0xFFF);
        *value = *(volatile uint16_t*)(dev->mmio + reg);
        return;
    }

    reg = (0x80000000 | (dev->pmio << 8) | (offset & 0xFC));
    outportl(CONFIG_ADDR, reg);
    *value = inportw(CONFIG_DATA + (offset & 2));
}

void pci_read_dword(pci_dev_t *dev, int offset, uint32_t *value)
{
    uint32_t reg;

    if(dev->mmio)
    {
        reg = (offset & 0xFFF);
        *value = *(volatile uint32_t*)(dev->mmio + reg);
        return;
    }

    reg = (0x80000000 | (dev->pmio << 8) | (offset & 0xFC));
    outportl(CONFIG_ADDR, reg);
    *value = inportl(CONFIG_DATA);
}

void pci_write_byte(pci_dev_t *dev, int offset, uint8_t value)
{
    uint32_t reg;

    if(dev->mmio)
    {
        reg = (offset & 0xFFF);
        *(volatile uint8_t*)(dev->mmio + reg) = value;
        return;
    }

    reg = (0x80000000 | (dev->pmio << 8) | (offset & 0xFC));
    outportl(CONFIG_ADDR, reg);
    outportb(CONFIG_DATA + (offset & 3), value);
}

void pci_write_word(pci_dev_t *dev, int offset, uint16_t value)
{
    uint32_t reg;

    if(dev->mmio)
    {
        reg = (offset & 0xFFF);
        *(volatile uint16_t*)(dev->mmio + reg) = value;
        return;
    }

    reg = (0x80000000 | (dev->pmio << 8) | (offset & 0xFC));
    outportl(CONFIG_ADDR, reg);
    outportw(CONFIG_DATA + (offset & 2), value);
}

void pci_write_dword(pci_dev_t *dev, int offset, uint32_t value)
{
    uint32_t reg;

    if(dev->mmio)
    {
        reg = (offset & 0xFFF);
        *(volatile uint32_t*)(dev->mmio + reg) = value;
        return;
    }

    reg = (0x80000000 | (dev->pmio << 8) | (offset & 0xFC));
    outportl(CONFIG_ADDR, reg);
    outportl(CONFIG_DATA, value);
}

void pci_read_bar(pci_dev_t *dev, int n, pci_bar_t *bar)
{
    uint32_t low, high, value;
    uint64_t address, size;
    uint8_t ofl, ofh, type;

    ofl = 0x10 + (0x04 * n);
    ofh = ofl + 0x04;
    pci_read_dword(dev, ofl, &low);

    // Check for empty address
    if(low == 0)
    {
        bar->type = BAR_ZERO;
        bar->value = 0;
        bar->size = 0;
        return;
    }

    // Determine BAR value
    if(low & 0x01)
    {
        address = (low & 0xFFFFFFFC);
        type = (BAR_PMIO | BAR_16BIT);
    }
    else
    {
        if((low & 0x06) == 0x06)
        {
            pci_read_dword(dev, ofh, &high);
            type = (BAR_MMIO | BAR_64BIT);
            address = high;
        }
        else
        {
            type = (BAR_MMIO | BAR_32BIT);
            address = 0;
        }

        address = (address << 32);
        address = (address | (low & 0xFFFFFFF0));
    }

    // Determine BAR size
    if(type & BAR_64BIT)
    {
        pci_write_dword(dev, ofh, 0xFFFFFFFF);
        pci_read_dword(dev, ofh, &value);
        pci_write_dword(dev, ofh, high);
        size = value;
    }
    else
    {
        size = 0xFFFFFFFF;
    }

    pci_write_dword(dev, ofl, 0xFFFFFFFF);
    pci_read_dword(dev, ofl, &value);
    pci_write_dword(dev, ofl, low);

    if(type & BAR_PMIO)
    {
        value = ((value & 0xFFFC) | 0xFFFF0000);
    }
    else
    {
        value &= 0xFFFFFFF0;
    }

    size = ((size << 32) | value);
    size = (~size) + 1;

    // Store results
    bar->type = type;
    bar->value = address;
    bar->size = size;
}

void pci_enable_bm(pci_dev_t *dev)
{
    uint16_t value;
    pci_read_word(dev, PCI_COMMAND, &value);
    value |= 0x004;
    pci_write_word(dev, PCI_COMMAND, value);
}

void pci_disable_bm(pci_dev_t *dev)
{
    uint16_t value;
    pci_read_word(dev, PCI_COMMAND, &value);
    value &= ~0x004;
    pci_write_word(dev, PCI_COMMAND, value);
}

void pci_enable_int(pci_dev_t *dev)
{
    uint16_t value;
    pci_read_word(dev, PCI_COMMAND, &value);
    value &= ~0x400;
    pci_write_word(dev, PCI_COMMAND, value);
}

void pci_disable_int(pci_dev_t *dev)
{
    uint16_t value;
    pci_read_word(dev, PCI_COMMAND, &value);
    value |= 0x400;
    pci_write_word(dev, PCI_COMMAND, value);
}
