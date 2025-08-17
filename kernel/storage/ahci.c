// #include <kernel/storage/partmgr.h>
#include <kernel/storage/ahci.h>
#include <kernel/vfs/devfs.h>
#include <kernel/mem/mmio.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/x86/irq.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>
#include <stdio.h>

static int bus_id = -1;

static int find_cmd_slot(ahci_dev_t *dev)
{
    uint32_t bits;
    uint8_t ncs;

    bits = (dev->port->sact | dev->port->ci);
    ncs = dev->host->ncs;

    for(int i = 0; i < ncs; i++)
    {
        if((bits & (1 << i)) == 0)
        {
            return i;
        }
    }

    return -ENXIO;
}

static void ahci_start_cmd(hba_port_t *port)
{
    while(port->cmd & PxCMD_CR);
    port->cmd |= PxCMD_FRE;
    port->cmd |= PxCMD_ST;
}

static void ahci_stop_cmd(hba_port_t *port)
{
    port->cmd &= ~PxCMD_ST;
    port->cmd &= ~PxCMD_FRE;
    while(port->cmd & (PxCMD_FR | PxCMD_CR));
}

static void ahci_handler(int gsi, void *data)
{
    uint32_t bits, status, is;
    ahci_host_t *host;
    ahci_dev_t *dev;
    hba_port_t *port;

    host = data;
    is = host->hba->is;
    bits = (is & host->hba->pi);

    for(int i = 0; i < host->np; i++)
    {
        dev  = host->dev + i;
        port = dev->port;

        if(port == 0)
        {
            continue;
        }

        if(bits & (1 << dev->id))
        {
            status = port->is;
            port->is = status;

            if(status & dev->irq.type)
            {
                dev->irq.status = status;
                dev->irq.flag = 1;

                if(dev->irq.signal)
                {
                    thread_signal(dev->wk.thread);
                }

                dev->irq.type = 0;
                dev->irq.signal = 0;
            }
        }
    }

    host->hba->is = is;
}

static int ahci_rebase(ahci_dev_t *dev)
{
    uint64_t virt, phys;
    hba_port_t *port;
    uint8_t ncs;

    // Set variables
    port = dev->port;
    ncs = dev->host->ncs;

    // Stop port
    ahci_stop_cmd(port);

    // Allocate memory
    if(mmio_alloc_uc_region(12288, 1024, &virt, &phys))
    {
        return -ENOMEM;
    }
    else
    {
        memset((void*)virt, 0, 12288);
    }

    // Set command list base (1 KB)
    port->clb = phys;
    dev->clb = (void*)virt;
    virt += 1024;
    phys += 1024;

    // Set FIS receive (256 bytes)
    port->fb = phys;
    dev->fb = (void*)virt;
    virt += 256;
    phys += 256;

    // Set command tables (256 bytes each)
    dev->ctb = (void*)virt;
    for(int i = 0; i < ncs; i++)
    {
        dev->clb[i].ctba = phys + (i * 256);
    }

    // Start port
    ahci_start_cmd(port);

    // Success
    return 0;
}

static int ahci_identify_device(ahci_dev_t *dev)
{
    fis_reg_h2d_t *fis;
    hba_chdr_t *clb;
    hba_ctbl_t *ctb;
    ata_info_t info;
    uint8_t slot;

    // Find command slot
    slot = find_cmd_slot(dev);
    if(slot < 0)
    {
        return -EBUSY;
    }

    clb = dev->clb + slot;
    ctb = dev->ctb + slot;

    // Construct FIS
    fis = (void*)ctb->cfis;
    fis->type = FIS_TYPE_REG_H2D;
    fis->pmport = 0;
    fis->rsv0 = 0;
    fis->c = 1;
    fis->command = (dev->atapi ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY);
    fis->feature0 = 0;
    fis->lba0 = 0;
    fis->device = 0;
    fis->lba1 = 0;
    fis->feature1 = 0;
    fis->count = 0;
    fis->icc = 0;
    fis->control = 0;
    fis->rsv1 = 0;

    // Add command to list
    clb->cfl = sizeof(*fis)/sizeof(uint32_t);
    clb->write = 0;
    clb->prdtl = 1;

    ctb->prdt->dba = dev->dma.phys;
    ctb->prdt->dbc = 511;
    ctb->prdt->ioc = 1;

    // Interrupt type
    dev->irq.flag = 0;
    dev->irq.type = PxIS_PSS;
    dev->irq.signal = 0;

    // Issue command
    dev->port->ci |= (1 << slot);

    // Wait for interrupt
    while(dev->irq.flag == 0);
    if(dev->irq.status & PxIS_TFES)
    {
        kp_info( "ahci", "TFES flag set");
        return -EIO;
    }

    // Decode identify
    libata_identify((uint16_t*)dev->dma.virt, &info);
    libata_print(&info, "ahci", bus_id, dev->id);
    dev->disk = info;

    return 0;
}

static int ahci_rw_dma(ahci_dev_t *dev, int rw, uint64_t lba, uint32_t count)
{
    fis_reg_h2d_t *fis;
    hba_chdr_t *clb;
    hba_ctbl_t *ctb;
    uint8_t slot;

    // Find command slot
    slot = find_cmd_slot(dev);
    if(slot < 0)
    {
        return -EBUSY;
    }

    clb = dev->clb + slot;
    ctb = dev->ctb + slot;

    // Construct FIS
    fis = (void*)ctb->cfis;
    fis->type = FIS_TYPE_REG_H2D;
    fis->pmport = 0;
    fis->rsv0 = 0;
    fis->c = 1;
    fis->command = ATA_CMD_READ_DMA_EXT;
    fis->feature0 = 0;
    fis->lba0 = (lba & 0xFFFFFFFF);
    fis->device = (1 << 6);
    fis->lba1 = ((lba >> 32) & 0xFFFF);
    fis->feature1 = 0;
    fis->count = count;
    fis->icc = 0;
    fis->control = 0;
    fis->rsv1 = 0;

    // Add command to list
    clb->cfl = sizeof(*fis) / 4;
    clb->write = 0;
    clb->prdtl = 1;

    ctb->prdt->dba = dev->dma.phys;
    ctb->prdt->dbc = (count * 512) - 1;
    ctb->prdt->ioc = 1;

    // Interrupt type
    dev->irq.flag = 0;
    dev->irq.type = PxIS_DHRS;
    dev->irq.signal = 1;

    // Issue command
    dev->port->ci |= (1 << slot);

    // Wait for interrupt
    thread_wait();
    if(dev->irq.status & PxIS_TFES)
    {
        kp_info( "ahci", "TFES flag set");
        return -EIO;
    }

    return 0;
}

static int ahci_read_core(void *dp, size_t lba, size_t count, void *buf)
{
    int status;
    ahci_dev_t *dev;

    dev = dp;
    status = ahci_rw_dma(dev, 0, lba, count);
    memcpy(buf, (void*)dev->dma.virt, count*512);

    return status;
}

static int ahci_read(file_t *file, size_t size, void *buf)
{
    ahci_dev_t *dev;
    int status;
// TODO: need to decide if we are working in bytes or blocks for block devices (size and seek)
    dev = file->data;
    status = libata_queue(&dev->wk, dev, 0, file->seek, size, buf);

    return status;
}

static int ahci_init_port(ahci_dev_t *dev, uint32_t sig, devfs_t *parent)
{
    uint64_t virt, phys;
    devfs_t *entry;
    char name[16];
    int status;

    // Not supported
    if((sig == AHCI_SIG_SEMB) || (sig == AHCI_SIG_PM))
    {
        return -ENOTSUP;
    }

    // ATAPI device
    if(sig == AHCI_SIG_ATAPI)
    {
        dev->atapi = 1;
    }

    // Rebase
    status = ahci_rebase(dev);
    if(status < 0)
    {
        return status;
    }

    // Allocate memory
    status = mmio_alloc_uc_region(ATA_DMA_SIZE, 2, &virt, &phys);
    if(status < 0)
    {
        return status;
    }

    dev->dma.virt = virt;
    dev->dma.phys = phys;
    dev->dma.size = ATA_DMA_SIZE;

    // Enable interrupts
    dev->port->ie = 0x3;

    // Identify device
    status = ahci_identify_device(dev);
    if(status < 0)
    {
        return status;
    }

    // Setup worker thread
    dev->irq.signal = 0;
    dev->wk.read = ahci_read_core;
    dev->wk.write = 0;
    sprintf(name, "ahci%ddisk%d", bus_id, dev->id);
    libata_start_worker(&dev->wk, name);

    // Register disk
    static devfs_ops_t ops = {
        .open = 0,
        .close = 0,
        .read = ahci_read,
        .seek = 0,
        .ioctl = 0
    };

    stat_t stat = {
        .size = dev->disk.bps * dev->disk.sectors,
        .blksz = dev->disk.bps,
        .blocks = dev->disk.sectors
    };

    sprintf(name, "disk%d", dev->id);
    entry = devfs_register(parent, name, &ops, dev, I_BLOCK, &stat);

    if(dev->atapi == 0)
    {
        // partmgr_read_table(parent, entry);
    }

    return 0;
}

static int ahci_alloc_irq_vectors(pci_dev_t *dev, ahci_host_t *host)
{
    int status, vector;

    status = pci_alloc_irq_vectors(dev, 1, 1);
    if(status < 0)
    {
        kp_error("ahci", "failed to allocated IRQ vectors (status %d)", status);
        return status;
    }

    vector = pci_irq_vector(dev, 0);
    irq_request(vector, ahci_handler, host);

    return 0;
}

void ahci_init_hba(pci_dev_t *pcidev, hba_mem_t *hba)
{
    uint32_t pi, ssts, sig;
    uint8_t np, ipm, det;
    uint64_t flags, tmp;
    ahci_host_t *host;
    ahci_dev_t *dev;
    devfs_t *parent;
    char name[16];
    cap_t cap;

    // Increment bus number
    bus_id++;

    // Create folder
    sprintf(name, "ahci%d", bus_id);
    parent = devfs_mkdir(0, name);

    // AHCI enable
    if((hba->ghc & GHC_AE) == 0)
    {
        hba->ghc |= GHC_AE;
    }

    // Capabilities
    flags = hba->cap2;
    flags = ((flags << 32) | hba->cap);
    memcpy(&cap, &flags, sizeof(cap));
    np = cap.np + 1;

    // BIOS handoff
    if(cap.boh)
    {
        kp_info( "ahci", "BIOS handoff not implemented");
        return;
    }

    // Check ports
    pi = hba->pi;
    if(pi == 0)
    {
        return;
    }

    // Allocate memory
    tmp = sizeof(ahci_host_t) + np*sizeof(ahci_dev_t);
    host = kzalloc(tmp);
    if(host == 0)
    {
        return;
    }
    dev = (ahci_dev_t*)(host+1);

    // Store information
    host->cap = cap;
    host->np = np;
    host->ncs = cap.ncs + 1;
    host->hba = hba;
    host->dev = dev;

    // Enable interrupts
    ahci_alloc_irq_vectors(pcidev, host);
    hba->ghc |= GHC_IE;
    hba->is = 0xFFFFFFFF;

    // Print information
    kp_info( "ahci", "ahci%d: version: %06x, ports: %d, ncs: %d", bus_id, hba->vs, np, host->ncs);
    kp_info( "ahci", "ahci%d: flags: %016lx ", bus_id, flags);

    // Initialize all devices
    for(int i = 0; i < 32; i++)
    {
        if(pi & (1 << i))
        {
            dev->id = i;
            dev->host = host;
            dev->port = hba->port + i;
            dev->atapi = 0;

            ssts = dev->port->ssts;
            sig  = dev->port->sig;
            ipm  = ((ssts >> 8) & 0x0F);
            det  = (ssts & 0x0F);

            // Present and active
            if((det == 3) && (ipm == 1))
            {
                ahci_init_port(dev, sig, parent);
            }

            dev++;
        }
    }
}

void ahci_init(pci_dev_t *dev)
{
    pci_bar_t bar5;
    hba_mem_t *hba;

    // Read BAR
    pci_read_bar(dev, 5, &bar5);

    // Check BAR
    if((bar5.type & BAR_MMIO) == 0)
    {
        return;
    }

    // Enable bus mastering
    pci_enable_bm(dev);

    // Initialize device
    hba = (void*)vmm_phys_to_virt(bar5.value);
    ahci_init_hba(dev, hba);
}
