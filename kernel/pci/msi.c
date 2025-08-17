#include <kernel/acpi/acpi.h>
#include <kernel/pci/pci.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/x86/irq.h>
#include <kernel/errno.h>

// MSI-X supports allocating any number of vectors (up to 2048 in total)
// MSI only supports allocating vectors in powers of two, with the
// options being (1, 2, 4, 8, 16, 32). These vectors must be allocated
// consecutively and they must be properly aligned.

static void x86_msi_data(uint64_t *addr, uint64_t *data, int apic_id, int vector)
{
    // Bit 20-31: Local APIC address (normally 0xFEE)
    // Bit 12-19: Destination (APIC ID)
    // Bit 3:     Redirection hint indication (used for lowest priority mode)
    // Bit 2:     Destination mode (0 = physical, 1 = logical)
    *addr = acpi_lapic_address() | (apic_id << 12);

    // Bit 15:   Trigger mode (0 = edge, 1 = level)
    // Bit 14:   Level for trigger mode (0 = low, 1 = high), only relevant for level trigger mode
    // Bit 8-10: Delivery mode (0 = fixed, 1 = lowest priority)
    // Bit 0-7:  Interrupt vector (bit 0-7)
    *data = vector;
}

static void pci_enable_msix(pci_dev_t *dev)
{
    uint16_t offset = dev->msix.offset;
    uint16_t msgctl;

    pci_read_word(dev, offset + 2, &msgctl);
    msgctl |= (1 << 15);
    pci_write_word(dev, offset + 2, msgctl);
}

static void pci_disable_msix(pci_dev_t *dev)
{
    uint16_t offset = dev->msix.offset;
    uint16_t msgctl;

    pci_read_word(dev, offset + 2, &msgctl);
    msgctl &= ~(1 << 15);
    pci_write_word(dev, offset + 2, msgctl);
}

static void pci_configure_msix(pci_dev_t *dev, int enable)
{
    volatile uint32_t *table;
    uint64_t addr, data;
    int apic_id, vector;

    addr = vmm_phys_to_virt(dev->msix.table);
    table = (volatile uint32_t*)addr;

    if(!enable)
    {
        pci_disable_msix(dev);
    }

    for(int i = 0; i < dev->numvecs; i++)
    {
        if(enable)
        {
            vector = dev->vecs[i].vector;
            apic_id = irq_affinity(vector);
            x86_msi_data(&addr, &data, apic_id, vector);
        }
        else
        {
            addr = 0;
            data = 0;
        }

        table[4*i+0] = addr;
        table[4*i+1] = addr >> 32;
        table[4*i+2] = data;
        table[4*i+3] = data >> 32;
    }

    if(enable)
    {
        pci_enable_msix(dev);
    }
}

static void pci_enable_msi(pci_dev_t *dev, int mme)
{
    uint16_t offset = dev->msi.offset;
    uint16_t msgctl;

    pci_read_word(dev, offset + 2, &msgctl);
    msgctl = (msgctl & 0xFF8E) | (mme << 4) | 1;
    pci_write_word(dev, offset + 2, msgctl);
}

static void pci_disable_msi(pci_dev_t *dev)
{
    uint16_t offset = dev->msi.offset;
    uint16_t msgctl;

    pci_read_word(dev, offset + 2, &msgctl);
    msgctl = (msgctl & 0xFF8E);
    pci_write_word(dev, offset + 2, msgctl);
}

static void pci_configure_msi(pci_dev_t *dev, int enable)
{
    uint64_t addr, data;
    int apic_id, vector;
    uint32_t offset;

    if(enable)
    {
        vector = dev->vecs->vector;
        apic_id = irq_affinity(vector);
        x86_msi_data(&addr, &data, apic_id, vector);
    }
    else
    {
        addr = 0;
        data = 0;
        pci_disable_msi(dev);
    }

    offset = dev->msi.offset + 4;
    pci_write_dword(dev, offset, addr);
    offset += 4;
    if(dev->msi.cap_64bit)
    {
        pci_write_dword(dev, offset, addr >> 32);
        offset += 4;
    }
    pci_write_word(dev, offset, data);

    if(enable)
    {
        offset = __builtin_ctz(dev->numvecs);
        pci_enable_msi(dev, offset);
    }
}

int pci_alloc_irq_vectors(pci_dev_t *dev, size_t minvecs, size_t maxvecs)
{
    pci_irq_t *vecs;
    int numvecs = 0;
    int vector;

    if(dev->vecs)
    {
        return -EBUSY;
    }

    if(!minvecs)
    {
        return -EINVAL;
    }

    if(minvecs > maxvecs)
    {
        return -EINVAL;
    }

    if(dev->msix.offset)
    {
        if(minvecs > dev->msix.size)
        {
            return -ENOSPC;
        }
        if(maxvecs > dev->msix.size)
        {
            maxvecs = dev->msix.size;
        }
    }
    else if(dev->msi.offset)
    {
        if(minvecs > dev->msi.size)
        {
            return -ENOSPC;
        }

        if(maxvecs > dev->msi.size)
        {
            maxvecs = dev->msi.size;
        }

        if(maxvecs > 16)
        {
            maxvecs = 32;
        }
        else if(maxvecs > 8)
        {
            maxvecs = 16;
        }
        else if(maxvecs > 4)
        {
            maxvecs = 8;
        }
        else if(maxvecs > 2)
        {
            maxvecs = 4;
        }

        if(minvecs > 16)
        {
            minvecs = 32;
        }
        else if(minvecs > 8)
        {
            minvecs = 16;
        }
        else if(minvecs > 4)
        {
            minvecs = 8;
        }
        else if(minvecs > 2)
        {
            minvecs = 4;
        }
    }
    else
    {
        if(dev->gsi < 0 || minvecs > 1)
        {
            return -ENOSPC;
        }
        maxvecs = 1;
    }

    vecs = kcalloc(maxvecs, sizeof(*vecs));
    if(!vecs)
    {
        return -ENOMEM;
    }

    if(dev->msix.offset)
    {
        for(int i = 0; i < maxvecs; i++)
        {
            vector = irq_alloc_msi_vectors(1);
            if(vector < 0)
            {
                break;
            }
            vecs[i].vector = vector;
            vecs[i].flags = PCI_IRQ_MSIX;
            numvecs++;
        }
    }
    else if(dev->msi.offset)
    {
        vector = irq_alloc_msi_vectors(maxvecs);
        if(vector < 0)
        {
            vector = irq_alloc_msi_vectors(minvecs);
            if(vector > 0)
            {
                numvecs = minvecs;
            }
        }
        else
        {
            numvecs = maxvecs;
        }

        for(int i = 0; i < numvecs; i++)
        {
            vecs[i].vector = vector + i;
            vecs[i].flags = PCI_IRQ_MSI;
        }
    }
    else
    {
        vector = irq_alloc_gsi_vector(dev->gsi);
        if(vector > 0)
        {
            vecs->vector = vector;
            vecs->flags = PCI_IRQ_GSI;
            numvecs++;
        }
    }

    if(numvecs < minvecs)
    {
        for(int i = 0; i < numvecs; i++)
        {
            irq_free_vector(vecs[i].vector);
        }
        kfree(vecs);
        return -ENOSPC;
    }

    dev->vecs = vecs;
    dev->numvecs = numvecs;

    if(dev->msix.offset)
    {
        pci_configure_msix(dev, 1);
    }
    else if(dev->msi.offset)
    {
        pci_configure_msi(dev, 1);
    }

    return numvecs;
}

void pci_free_irq_vectors(pci_dev_t *dev)
{
    if(!dev->numvecs)
    {
        return;
    }

    if(dev->msix.offset)
    {
        pci_configure_msix(dev, 0);
    }
    else if(dev->msi.offset)
    {
        pci_configure_msi(dev, 0);
    }

    for(int i = 0; i < dev->numvecs; i++)
    {
        irq_free_vector(dev->vecs[i].vector);
    }

    kfree(dev->vecs);
    dev->numvecs = 0;
    dev->vecs = 0;
}

int pci_irq_vector(pci_dev_t *dev, size_t num)
{
    if(num > dev->numvecs)
    {
        return -EINVAL;
    }
    return dev->vecs[num].vector;
}
