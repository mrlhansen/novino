#include <kernel/storage/blkdev.h>
#include <kernel/storage/ahci.h>
#include <kernel/vfs/devfs.h>
#include <kernel/mem/mmio.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/x86/irq.h>
#include <kernel/time/time.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>
#include <stdio.h>

static int bus_id = -1;

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

            dev->irq.status = status;
            dev->irq.flag = 1;

            if(dev->irq.signal)
            {
                 if(status & dev->irq.type)
                 {
                    thread_signal(dev->wk.thread);
                    dev->irq.signal = 0;
                 }
            }
        }
    }

    host->hba->is = is;
}

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

    return -ENOSPC;
}

static int ahci_submit_command(ahci_dev_t *dev, ahci_fis_t *fis, ahci_prd_t *prd, int numprd, int write)
{
    hba_chdr_t *clb;
    hba_ctbl_t *ctb;
    int slot;

    if(fis->type != FIS_TYPE_REG_H2D)
    {
        kp_info("ahci", "unimplemented FIS type: %d", fis->type);
        return -EFAIL;
    }

    fis_reg_h2d_t h2d = {
        .type = fis->type,
        .pmport = 0,
        .rsv0 = 0,
        .c = 1,
        .command = fis->command,
        .feature0 = fis->features,
        .lba0 = fis->lba,
        .device = fis->device,
        .lba1 = (fis->lba >> 24),
        .feature1 = 0,
        .count = fis->count,
        .icc = 0,
        .control = 0,
        .rsv1 = 0,
    };

    slot = find_cmd_slot(dev);
    if(slot < 0)
    {
        return slot;
    }

    clb = dev->clb + slot;
    ctb = dev->ctb + slot;

    memcpy(ctb->cfis, &h2d, sizeof(h2d));
    if(fis->atapi)
    {
        clb->atapi = 1;
        memcpy(ctb->acmd, fis->packet, 16);
    }

    clb->cfl = sizeof(h2d) / 4;
    clb->write = write;
    clb->prdtl = numprd;

    for(int i = 0; i < numprd; i++)
    {
        ctb->prdt[i].dba = prd->phys;
        ctb->prdt[i].dbc = prd->size - 1;
        ctb->prdt[i].ioc = prd->ioc;
    }

    dev->irq.flag = 0;
    dev->port->ci |= (1 << slot);

    return 0;
}

static int ahci_poll_event(ahci_dev_t *dev, int type)
{
    uint64_t now, end;

    now = system_timestamp();
    end = now + 100000000; // 100ms

    while(now < end)
    {
        if(dev->irq.flag)
        {
            if(dev->irq.status & PxIS_TFES)
            {
                return -EIO;
            }
            else if(dev->irq.status & type)
            {
                return 0;
            }
        }
        now = system_timestamp();
    }

    return -ETMOUT;
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

static int ahci_identify_device(ahci_dev_t *dev)
{
    ahci_fis_t fis;
    ahci_prd_t prd;
    int status;

    fis.type = FIS_TYPE_REG_H2D;
    fis.atapi = 0;
    fis.command = (dev->atapi ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY);
    fis.features = 0;
    fis.device = 0;
    fis.lba = 0;
    fis.count = 0;

    prd.phys = dev->dma.phys;
    prd.size = 512;
    prd.ioc = 1;

    status = ahci_submit_command(dev, &fis, &prd, 1, 0);
    if(status < 0)
    {
        return status;
    }

    status = ahci_poll_event(dev, PxIS_PSS);
    if(status < 0)
    {
        return status;
    }

    libata_identify((uint16_t*)dev->dma.virt, &dev->disk);
    libata_print(&dev->disk, "ahci", dev->host->bus, dev->id);

    return 0;
}

static int ahci_atapi_packet(ahci_dev_t *dev, uint8_t *packet, int size, int poll)
{
    ahci_fis_t fis;
    ahci_prd_t prd;
    int numprd = 0;
    int status;

    fis.type = FIS_TYPE_REG_H2D;
    fis.atapi = 1;
    fis.command = ATA_CMD_PACKET;
    fis.features = 0;
    fis.device = 0;
    fis.lba = 0;
    fis.count = 0;

    if(size > 0)
    {
        prd.phys = dev->dma.phys;
        prd.size = size;
        prd.ioc = 1;

        // Data transfer is via DMA (ATA-8, 7.22 PACKET)
        fis.features |= 1;

        // DMA direction (ATA-8, 7.22 PACKET)
        if(dev->disk.flags & ATA_FLAG_DMADIR)
        {
            fis.features |= 4;
        }

        numprd++;
    }

    for(int i = 0; i < 16; i++)
    {
        fis.packet[i] = packet[i];
    }

    status = ahci_submit_command(dev, &fis, &prd, numprd, 0);
    if(status < 0)
    {
        return status;
    }

    if(!poll)
    {
        return 0;
    }

    status = ahci_poll_event(dev, PxIS_DHRS);
    if(status < 0)
    {
        return status;
    }

    return 0;
}

static int ahci_atapi_read_capacity(ahci_dev_t *dev)
{
    int bps, sectors, status;
    uint8_t packet[16] = {0};
    uint8_t *data;

    packet[0] = ATAPI_CMD_READ_CAPACITY;
    status = ahci_atapi_packet(dev, packet, 8, 1);
    if(status < 0)
    {
        return status;
    }

    data = (void*)dev->dma.virt;

    sectors = data[0];
    sectors = (sectors << 8) | data[1];
    sectors = (sectors << 8) | data[2];
    sectors = (sectors << 8) | data[3];

    bps = data[4];
    bps = (bps << 8) | data[5];
    bps = (bps << 8) | data[6];
    bps = (bps << 8) | data[7];

    dev->disk.sectors = sectors + 1;
    dev->disk.lss = bps;
    dev->disk.pss = bps;

    return 0;
}

static int ahci_atapi_request_sense(ahci_dev_t *dev)
{
    uint8_t packet[16] = {0};
    uint8_t *data;
    int asc;
    int key;

    data = (void*)dev->dma.virt;
    data[0] = 0;

    packet[0] = ATAPI_CMD_REQUEST_SENSE;
    packet[4] = 18;
    ahci_atapi_packet(dev, packet, 18, 1);

    if(data[0] == 0)
    {
        return -EIO;
    }

    key = (data[2] & 0x0F);
    asc = data[12];

    if(key)
    {
        return (asc << 8) | key;
    }

    return 0;
}

static int ahci_atapi_test_unit_ready(ahci_dev_t *dev)
{
    uint8_t packet[16] = {0};
    int status;

    packet[0] = ATAPI_CMD_TEST_UNIT_READY;
    status = ahci_atapi_packet(dev, packet, 0, 1);
    if(status < 0)
    {
        return status;
    }

    return 0;
}

static int ahci_atapi_status_check(ahci_dev_t *dev)
{
    int status;

    status = ahci_atapi_test_unit_ready(dev);
    if(!status)
    {
        if(dev->status == BLKDEV_MEDIA_CHANGED)
        {
            return -EBUSY;
        }
        return 0;
    }

    status = ahci_atapi_request_sense(dev);
    if(status < 1)
    {
        return status;
    }

    if(status == 0x3A02)
    {
        if(dev->status != BLKDEV_MEDIA_ABSENT)
        {
            dev->status = BLKDEV_MEDIA_ABSENT;
            kp_debug("ahci", "ahci%d.%d: medium not present", dev->host->bus, dev->id);
        }
        return -ENOMEDIUM;
    }
    else if(status == 0x2806)
    {
        if(dev->status != BLKDEV_MEDIA_CHANGED)
        {
            dev->status = BLKDEV_MEDIA_CHANGED;
            kp_debug("ahci", "ahci%d.%d: medium has changed", dev->host->bus, dev->id);
        }
        status = ahci_atapi_read_capacity(dev);
        if(!status)
        {
            kp_debug("ahci", "ahci%d.%d: detected capacity %d sectors", dev->host->bus, dev->id, dev->disk.sectors);
        }
    }

    if(dev->status == BLKDEV_MEDIA_CHANGED)
    {
        return -EBUSY;
    }

    return 0;
}

static int ahci_atapi_read_dma(ahci_dev_t *dev, uint64_t lba, uint32_t count)
{
    uint8_t packet[16] = {0};
    int status;

    status = ahci_atapi_status_check(dev);
    if(status < 0)
    {
        return status;
    }

    packet[0] = ATAPI_CMD_READ;
    packet[2] = ((lba >> 24) & 0xFF);
    packet[3] = ((lba >> 16) & 0xFF);
    packet[4] = ((lba >> 8) & 0xFF);
    packet[5] = ((lba >> 0) & 0xFF);
    packet[6] = ((count >> 24) & 0xFF);
    packet[7] = ((count >> 16) & 0xFF);
    packet[8] = ((count >> 8) & 0xFF);
    packet[9] = ((count >> 0) & 0xFF);

    dev->irq.type = PxIS_DHRS;
    dev->irq.signal = 1;

    status = ahci_atapi_packet(dev, packet, 2048 * count, 0);
    if(status < 0)
    {
        return status;
    }

    thread_wait();
    // PxIS_TFES set?

    return 0;
}

static int ahci_flush_cache(ahci_dev_t *dev)
{
    ahci_fis_t fis;
    int status;

    if((dev->disk.flags & ATA_FLAG_WCE) == 0)
    {
        return 0;
    }

    fis.type = FIS_TYPE_REG_H2D;
    fis.atapi = 0;
    fis.command = ATA_CMD_FLUSH_CACHE_EXT;
    fis.features = 0;
    fis.device = 0;
    fis.lba = 0;
    fis.count = 0;

    dev->irq.type = PxIS_DHRS;
    dev->irq.signal = 1;

    status = ahci_submit_command(dev, &fis, 0, 0, 0);
    if(status < 0)
    {
        return status;
    }

    thread_wait();
    // PxIS_TFES set?

    return 0;
}

static int ahci_rw_dma(ahci_dev_t *dev, int write, uint64_t lba, uint32_t count)
{
    ahci_fis_t fis;
    ahci_prd_t prd;
    int status;

    fis.type = FIS_TYPE_REG_H2D;
    fis.atapi = 0;
    fis.command = (write ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT);
    fis.features = 0;
    fis.device = (1 << 6); // LBA mode (ATA-8, 7.25 READ DMA EXT)
    fis.lba = lba;
    fis.count = count;

    prd.phys = dev->dma.phys;
    prd.size = dev->disk.lss * count; // TODO: we might need multiple PRDs
    prd.ioc = 1;

    dev->irq.type = PxIS_DHRS;
    dev->irq.signal = 1;

    status = ahci_submit_command(dev, &fis, &prd, 1, write);
    if(status < 0)
    {
        return status;
    }

    thread_wait();
    // PxIS_TFES set?

    if(write)
    {
        // TODO: consider making a blkdev flush function to avoid flushing on every write
        status = ahci_flush_cache(dev);
        if(status < 0)
        {
            kp_info("ahci", "flush failed");
        }
    }

    return 0;
}

static int ahci_read_core(void *dp, size_t lba, size_t count, void *buf)
{
    ahci_dev_t *dev = dp;
    int status;

    // TODO: take dma buffer size into account

    if(dev->atapi)
    {
        status = ahci_atapi_read_dma(dev, lba, count);
    }
    else
    {
        status = ahci_rw_dma(dev, 0, lba, count);
    }

    if(status < 0)
    {
        return status;
    }

    memcpy(buf, (void*)dev->dma.virt, dev->disk.lss * count);
    return 0;
}

static int ahci_read(void *data, size_t lba, size_t count, void *buf)
{
    ahci_dev_t *dev = data;
    return libata_queue(&dev->wk, dev, 0, lba, count, buf);
}

static int ahci_write_core(void *dp, size_t lba, size_t count, void *buf)
{
    ahci_dev_t *dev = dp;

    // TODO: take dma buffer size into account

    if(dev->atapi)
    {
        return -EIO;
    }

    memcpy((void*)dev->dma.virt, buf, dev->disk.lss * count);
    return ahci_rw_dma(dev, 1, lba, count);
}

static int ahci_write(void *data, size_t lba, size_t count, void *buf)
{
    ahci_dev_t *dev = data;
    return libata_queue(&dev->wk, dev, 1, lba, count, buf);
}

static int ahci_status(void *data, blkdev_t *blk, int ack)
{
    ahci_dev_t *dev = data;
    int status = 0;

    blk->bps = dev->disk.lss;
    blk->sectors = dev->disk.sectors;
    blk->flags = 0;

    if(!dev->atapi)
    {
        return 0;
    }

    blk->flags = BLKDEV_REMOVEABLE;
    status = ahci_atapi_status_check(dev);

    if(dev->status == BLKDEV_MEDIA_ABSENT)
    {
        blk->bps = 0;
        blk->sectors = 0;
        blk->flags |= BLKDEV_MEDIA_ABSENT;
    }
    else if(dev->status == BLKDEV_MEDIA_CHANGED)
    {
        blk->bps = dev->disk.lss;
        blk->sectors = dev->disk.sectors;
        blk->flags |= BLKDEV_MEDIA_CHANGED;

        if(ack)
        {
            dev->status = 0;
            status = 0;
        }
    }

    return status;
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
    dev->port->ie = PxIS_DHRS | PxIS_PSS | PxIS_TFES;

    // Identify device
    status = ahci_identify_device(dev);
    if(status < 0)
    {
        return status;
    }

    // ATAPI checks (might need a separate function)
    if(dev->atapi)
    {
        status = ahci_atapi_test_unit_ready(dev);
        if(!status)
        {
            status = ahci_atapi_request_sense(dev);
            if(!status)
            {
                ahci_atapi_read_capacity(dev);
            }
        }
    }

    // Setup worker thread
    dev->irq.signal = 0;
    dev->wk.read = ahci_read_core;
    dev->wk.write = ahci_write_core;
    sprintf(name, "ahci%d.%d", dev->host->bus, dev->id);
    libata_start_worker(&dev->wk, name);

    // Register disk
    static devfs_blk_t ops = {
        .status = ahci_status,
        .read = ahci_read,
        .write = ahci_write,
    };

    sprintf(name, "disk%d", dev->id);
    entry = devfs_block_register(parent, name, &ops, dev, 0);
    if(!entry)
    {
        return -ENOMEM;
    }

    if(dev->atapi)
    {
        blkdev_alloc(entry, 0, 0, false);
    }
    else
    {
        blkdev_alloc(entry, 0, 0, true);
    }

    return 0;
}

static int ahci_alloc_irq_vectors(pci_dev_t *dev, ahci_host_t *host)
{
    int status, vector;

    status = pci_alloc_irq_vectors(dev, 1, 1);
    if(status < 0)
    {
        kp_error("ahci", "failed to allocate IRQ vectors (status %d)", status);
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
        kp_info("ahci", "BIOS handoff not implemented");
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
    host->bus = bus_id;
    host->np = np;
    host->ncs = cap.ncs + 1;
    host->hba = hba;
    host->dev = dev;

    // Create devfs folder
    sprintf(name, "ahci%d", host->bus);
    parent = devfs_mkdir(0, name);

    // Enable interrupts
    ahci_alloc_irq_vectors(pcidev, host);
    hba->ghc |= GHC_IE;
    hba->is = 0xFFFFFFFF;

    // Print information
    kp_info("ahci", "ahci%d: version: %06x, ports: %d, ncs: %d", host->bus, hba->vs, np, host->ncs);
    kp_info("ahci", "ahci%d: flags: %016lx ", host->bus, flags);

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
