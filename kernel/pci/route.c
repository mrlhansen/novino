#include <kernel/x86/ioapic.h>
#include <kernel/mem/heap.h>
#include <kernel/x86/irq.h>
#include <kernel/pci/pci.h>
#include <kernel/debug.h>
#include <sabi/resources.h>
#include <sabi/pci.h>

static pci_dev_list_t *pci_find_parent(int bus)
{
    pci_dev_list_t *item = 0;

    while(item = pci_next_device(item), item)
    {
        if(item->bridge && (item->br_sbus == bus))
        {
            return item;
        }
    }

    return 0;
}

static void pci_bridge_prt(sabi_node_t *parent, int bus)
{
    pci_dev_list_t *item = 0;
    sabi_node_t *node;
    sabi_prt_t *prt;
    int status;

    while(item = pci_next_device(item), item)
    {
        if(item->bridge && (item->bus == bus) && (item->br_sbus != item->bus))
        {
            node = sabi_pci_find_device(parent, item->slot, item->fn);
            if(node)
            {
                pci_bridge_prt(node, item->br_sbus);
                status = sabi_pci_eval_prt(node, &prt);

                if(status < 0)
                {
                    continue;
                }

                item->br_prt = prt;
                item->br_prt_cnt = status;
            }
        }
    }
}

static void pci_host_prt()
{
    pci_dev_list_t *parent;
    sabi_node_t *node = 0;
    sabi_prt_t *prt;
    int bbn, status;

    while(node = sabi_pci_next_node(node), node)
    {
        bbn = sabi_pci_eval_bbn(node);
        status = sabi_pci_eval_prt(node, &prt);

        if(status < 0)
        {
            kp_panic("pci", "failed to evaluate prt for host");
        }

        parent = pci_find_parent(bbn);
        parent->br_prt = prt;
        parent->br_prt_cnt = status;

        pci_bridge_prt(node, bbn);
    }
}

static void pci_find_gsi(pci_dev_list_t *item)
{
    pci_dev_list_t *parent;
    sabi_prt_t *prt;
    int polarity, mode;
    int bus, slot, pin;
    int count, gsi;

    bus = item->bus;
    slot = item->slot;
    pin = item->pin-1;

    while(1)
    {
        parent = pci_find_parent(bus);
        prt = parent->br_prt;
        count = parent->br_prt_cnt;

        if(prt)
        {
            for(int i = 0; i < count; i++)
            {
                if((prt[i].slot == slot) && (prt[i].pin == pin))
                {
                    gsi = prt[i].gsi;
                    polarity = (prt[i].flags & SABI_IRQ_POLARITY) ? LOW : HIGH;
                    mode = (prt[i].flags & SABI_IRQ_MODE) ? EDGE : LEVEL;

                    item->dev.gsi = gsi;
                    irq_configure(gsi, gsi, polarity, mode);
                }
            }
            break;
        }
        else
        {
            bus = parent->bus;
            pin = (pin + slot) % 4;
            slot = parent->slot;
        }
    }
}

void pci_route_init()
{
    pci_dev_list_t *item = 0;

    // Route PCI interrupts
    pci_host_prt();

    // Assign GSI to device and initialize driver
    while(item = pci_next_device(item), item)
    {
        if(item->pin)
        {
            pci_find_gsi(item);
        }
        pci_find_driver(&item->dev);
    }
}
