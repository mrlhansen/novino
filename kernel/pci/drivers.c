#include <kernel/storage/ahci.h>
#include <kernel/net/rtl8139.h>
#include <kernel/usb/pci.h>

static const pci_drv_list_t clist[] = {
    {0x01, 0x06, ahci_init},
    {0x0C, 0x03, usb_init},
};

static const pci_drv_list_t dlist[] = {
    {0x10ec, 0x8139, rtl8139_init}
};

void pci_find_driver(pci_dev_t *dev)
{
    int first, second;
    int count;

    // Check in vendor/device list
    count = sizeof(dlist) / sizeof(pci_drv_list_t);
    for(int i = 0; i < count; i++)
    {
        first = (dlist[i].first == dev->vendor);
        second = (dlist[i].second == dev->device);
        if(first && second)
        {
            dlist[i].init(dev);
            return;
        }
    }

    // Check in classcode list
    count = sizeof(clist) / sizeof(pci_drv_list_t);
    for(int i = 0; i < count; i++)
    {
        first = (clist[i].first == dev->classcode);
        second = (clist[i].second == dev->subclass);
        if(first && second)
        {
            clist[i].init(dev);
            return;
        }
    }
}
