#include <kernel/acpi/acpi.h>
#include <kernel/acpi/mcfg.h>
#include <kernel/pci/pci.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>

static LIST_INIT(devices, pci_dev_list_t, link);

static char *pci_class[] = {
    "Unknown",
    "Storage",
    "Network",
    "Display",
    "Multimedia",
    "Memory",
    "Bridge",
    "Communication",
    "Peripheral",
    "Input",
    "Docking Station",
    "Processor",
    "Serial Bus",
    "Wireless",
    "Intelligent",
    "Satellite Communication",
    "Encryption",
    "Signal Processing",
    "Processing Accelerator",
    "Instrumentation",
};

pci_dev_list_t *pci_next_device(pci_dev_list_t *item)
{
    if(item)
    {
        return item->link.next;
    }
    else
    {
        return devices.head;
    }
}

static void pci_check_bridge(pci_dev_list_t *item)
{
    uint8_t sb;
    int cc, sc;

    cc = item->dev.classcode;
    sc = item->dev.subclass;

    item->bridge = 0;
    item->br_sbus = 0;
    item->br_prt = 0;
    item->br_prt_cnt = 0;

    if(cc == 0x06)
    {
        if(sc == 0x00) // Host bridge
        {
            item->br_sbus = item->bus;
            item->bridge = 1;
        }
        else if((sc == 0x04) || (sc == 0x09)) // PCI-to-PCI bridge
        {
            pci_read_byte(&item->dev, PCI_SECONDARY_BUS, &sb);
            item->br_sbus = sb;
            item->bridge = 1;
        }
    }
}

static void pci_check_msi(pci_dev_t *dev)
{
    uint16_t status, offset, msgctl;
    uint32_t value, bir, off;
    pci_bar_t bar;
    int capid;

    // unsupported by default
    dev->msi.offset = 0;
    dev->msix.offset = 0;

    // check that the capability list exists
    pci_read_word(dev, PCI_STATUS, &status);
    if((status & 0x10) == 0)
    {
        return;
    }

    // cap pointer
    pci_read_word(dev, PCI_CAP_OFFSET, &offset);
    offset = (offset & 0xFFFC);

    // loop over capabilities
    while(offset)
    {
        pci_read_word(dev, offset, &status);
        capid = (status & 0xFF);

        if(capid == 0x05) // MSI
        {
            pci_read_word(dev, offset + 2, &msgctl);
            dev->msi.offset    = offset;
            dev->msi.cap_pvm   = (msgctl >> 8) & 0x01;
            dev->msi.cap_64bit = (msgctl >> 7) & 0x01;
            dev->msi.cap_mm    = (msgctl >> 1) & 0x07;
            dev->msi.size      = (1 << dev->msi.cap_mm);
        }
        else if(capid == 0x11) // MSI-X
        {
            pci_read_word(dev, offset + 2, &msgctl);
            dev->msix.offset = offset;
            dev->msix.size   = (msgctl & 0x7FF) + 1;

            pci_read_dword(dev, offset + 4, &value);
            bir = (value & 0x07);
            off = (value & ~0x07);
            pci_read_bar(dev, bir, &bar);
            dev->msix.table = bar.value + off;

            pci_read_dword(dev, offset + 8, &value);
            bir = (value & 0x07);
            off = (value & ~0x07);
            pci_read_bar(dev, bir, &bar);
            dev->msix.pba = bar.value + off;
        }

        offset = (status >> 8);
    }
}

static void pci_enumerate(uint64_t mmio, int start, int end)
{
    pci_dev_list_t *item;
    uint8_t pin, type;
    pci_dev_t dev;

    for(int bus = start; bus < end; bus++)
    {
        for(int slot = 0; slot < 32; slot++)
        {
            for(int fn = 0; fn < 8; fn++)
            {
                dev.pmio = ((bus << 8) | (slot << 3) | fn);
                dev.mmio = 0;
                dev.gsi = -1;
                dev.vecs = 0;

                if(mmio)
                {
                    dev.mmio = mmio + (dev.pmio << 12);
                }

                // Check if device exists
                pci_read_word(&dev, PCI_VENDOR_ID, &dev.vendor);
                if(dev.vendor == 0xFFFF || dev.vendor == 0)
                {
                    if(fn == 0)
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }

                // Read information
                pci_read_word(&dev, PCI_DEVICE_ID, &dev.device);
                pci_read_byte(&dev, PCI_REVISION, &dev.revision);
                pci_read_byte(&dev, PCI_PROG_IF, &dev.prog_if);
                pci_read_byte(&dev, PCI_SUBCLASS, &dev.subclass);
                pci_read_byte(&dev, PCI_CLASSCODE, &dev.classcode);
                pci_read_byte(&dev, PCI_INTERRUPT_PIN, &pin);
                pci_read_byte(&dev, PCI_HEADER_TYPE, &type);

                // Add device
                item = kzalloc(sizeof(*item));
                if(item == 0)
                {
                    // error
                }

                item->seg = 0;
                item->bus = bus;
                item->slot = slot;
                item->fn = fn;
                item->pin = pin;
                item->dev = dev;

                pci_check_bridge(item);
                pci_check_msi(&item->dev);
                list_append(&devices, item);

                // Multifunction device
                if((fn == 0) && ((type & 0x80) == 0))
                {
                    break;
                }
            }
        }
    }
}

void pci_init()
{
    pci_dev_list_t *item;
    mcfg_entry_t *entry;
    uint64_t address;
    pci_dev_t *dev;
    char *name;
    int ns;

    address = acpi_mcfg_entries(&ns);
    entry = (void*)address;
    item = 0;

    // Enumerate PCI devices
    if(address == 0)
    {
        kp_info( "pci", "found conventional PCI bus");
        pci_enumerate(0, 0, 256);
    }
    else
    {
        kp_info( "pci", "found PCI express bus, %d segment group(s)", ns);
        for(int i = 0; i < ns; i++)
        {
            address = vmm_phys_to_virt(entry[i].base_address);
            kp_info( "pci", "segment %d, mmio %#lx, bus %d-%d", entry[i].segment_group, entry[i].base_address, entry[i].start_bus, entry[i].end_bus);
            pci_enumerate(address, entry[i].start_bus, entry[i].end_bus+1);
        }
    }

    // Print list of devices
    while(item = pci_next_device(item), item)
    {
        dev = &item->dev;
        name = pci_class[dev->classcode];
        kp_info( "pci", "%d:%d.%d: [%s] vendor: %04x, device: %04x", item->bus, item->slot, item->fn, name, dev->vendor, dev->device);
    }
}
