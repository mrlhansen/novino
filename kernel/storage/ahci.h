#pragma once

#include <kernel/storage/libata.h>
#include <kernel/pci/pci.h>
#include <kernel/types.h>

enum {
    AHCI_SIG_ATA   = 0x00000101, // SATA drive
    AHCI_SIG_ATAPI = 0xEB140101, // SATAPI drive
    AHCI_SIG_SEMB  = 0xC33C0101, // Enclosure management bridge
    AHCI_SIG_PM    = 0x96690101  // Port multiplier
};

enum {
    FIS_TYPE_REG_H2D    = 0x27, // Register FIS - host to device
    FIS_TYPE_REG_D2H    = 0x34, // Register FIS - device to host
    FIS_TYPE_DMA_ACT    = 0x39, // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP  = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA       = 0x46, // Data FIS - bidirectional
    FIS_TYPE_BIST       = 0x58, // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP  = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS   = 0xA1  // Set device bits FIS - device to host
};

enum {
    GHC_HR   = (1 << 0), // HBA reset
    GHC_IE   = (1 << 1), // Interrupt enable
    GHC_MRSM = (1 << 2), // MSI revert to single message
    GHC_AE   = (1 << 31) // AHCI enable
};

enum {
    PxCMD_ST    = (1 << 0),   // Start
    PxCMD_SUD   = (1 << 1),   // Spin-up device
    PxCMD_POD   = (1 << 2),   // Power on device
    PxCMD_CLO   = (1 << 3),   // Command list override
    PxCMD_FRE   = (1 << 4),   // FIS receive enable
    PxCMD_CCS   = (0xF << 8), // Current command slot
    PxCMD_MPSS  = (1 << 13),  // Mechanical presence switch state
    PxCMD_FR    = (1 << 14),  // FIS receive running
    PxCMD_CR    = (1 << 15),  // Command list running
    PxCMD_CPS   = (1 << 16),  // Cold presence state
    PxCMD_PMA   = (1 << 17),  // Port multiplier attached
    PxCMD_HPCP  = (1 << 18),  // Hot-plug capable port
    PxCMD_MPSP  = (1 << 19),  // Mechanical presence switch attached to port
    PxCMD_CPD   = (1 << 20),  // Cold presence detected
    PxCMD_ESP   = (1 << 21),  // External SATA port
    PxCMD_FSSCP = (1 << 22),  // FIS-based switching capable port
    PxCMD_APSTE = (1 << 23),  // Automatic partial to slumber transitions enabled
    PxCMD_ATAPI = (1 << 24),  // Device is ATAPI
    PxCMD_DLAE  = (1 << 25),  // Drive LED on ATAPI enable
    PxCMD_ALPE  = (1 << 26),  // Aggressive link power management enable
    PxCMD_ASP   = (1 << 27),  // Aggressive slumber partial
    PxCMD_ICC   = (0xF << 28) // Interface communication control
};

enum {
    PxIS_DHRS = (1 << 0),  // Device to Host Register FIS
    PxIS_PSS  = (1 << 1),  // PIO Setup FIS
    PxIS_DSS  = (1 << 2),  // DMA Setup FIS
    PxIS_SDBS = (1 << 3),  // Set Device Bits
    PxIS_UFS  = (1 << 4),  // Unknown FIS
    PxIS_DPS  = (1 << 5),  // Descriptor Processed
    PxIS_PCS  = (1 << 6),  // Port Connect Change Status
    PxIS_DMPS = (1 << 7),  // Device Mechanical Presence Status
    PxIS_PRCS = (1 << 22), // PhyRdy Change Status
    PxIS_IPMS = (1 << 23), // Incorrect Port Multiplier Status
    PxIS_OFS  = (1 << 24), // Overflow Status
    PxIS_INFS = (1 << 26), // Interface Non-fatal Error Status
    PxIS_IFS  = (1 << 27), // Interface Fatal Error Status
    PxIS_HBDS = (1 << 28), // Host Bus Data Error Status
    PxIS_HBFS = (1 << 29), // Host Bus Fatal Error Status
    PxIS_TFES = (1 << 30), // Task File Error Status
    PxIS_CPDS = (1 << 31), // Cold Port Detect Status
};

typedef struct {
    uint32_t np    : 5;  // Number of ports
    uint32_t sxs   : 1;  // Supports external SATA
    uint32_t ems   : 1;  // Enclosure management supported
    uint32_t cccs  : 1;  // Command completion coalescing supported
    uint32_t ncs   : 5;  // Number of command slots
    uint32_t psc   : 1;  // Partial state capable
    uint32_t ssc   : 1;  // Slumber state capable
    uint32_t pmd   : 1;  // PIO multiple DRQ block
    uint32_t fbss  : 1;  // FIS-based switching supported
    uint32_t spm   : 1;  // Supports port multiplier
    uint32_t sam   : 1;  // Supports AHCI mode only
    uint32_t rsv0  : 1;  // Reserved
    uint32_t iss   : 4;  // Interface speed support
    uint32_t sclo  : 1;  // Supports command list override
    uint32_t sal   : 1;  // Supports activity LED
    uint32_t salp  : 1;  // Supports aggressive link power management
    uint32_t sss   : 1;  // Supports staggered spin-up
    uint32_t smps  : 1;  // Supports mechanical presence switch
    uint32_t ssntf : 1;  // Supports S-notification register
    uint32_t sncq  : 1;  // Supports native command queuing
    uint32_t s64a  : 1;  // Supports 64-bit addressing
    uint32_t boh   : 1;  // BIOS/OS handoff
    uint32_t nvmp  : 1;  // NVMHCI Present
    uint32_t apst  : 1;  // Automatic partial to slumber transitions
    uint32_t sds   : 1;  // Supports device sleep
    uint32_t sadm  : 1;  // Supports aggressive device sleep management
    uint32_t deso  : 1;  // DevSleep entrance from slumber only
    uint32_t rsv1  : 26; // Reserved
} __attribute__((packed)) cap_t;

typedef volatile struct {
    uint64_t clb;       // Command list base address (1024 bytes aligned)
    uint64_t fb;        // FIS base address (256 bytes aligned)
    uint32_t is;        // Interrupt status
    uint32_t ie;        // Interrupt enable
    uint32_t cmd;       // Command and status
    uint32_t rsv0;      // Reserved
    uint32_t tfd;       // Task file data
    uint32_t sig;       // Signature
    uint32_t ssts;      // SATA status (SCR0:SStatus)
    uint32_t sctl;      // SATA control (SCR2:SControl)
    uint32_t serr;      // SATA error (SCR1:SError)
    uint32_t sact;      // SATA active (SCR3:SActive)
    uint32_t ci;        // Command issue
    uint32_t sntf;      // SATA notification (SCR4:SNotification)
    uint32_t fbs;       // FIS-based switch control
    uint32_t rsv1[11];  // Reserved
    uint32_t vendor[4]; // Vendor specific
} __attribute__((packed)) hba_port_t;

typedef volatile struct {
    uint32_t cap;     // Host capability
    uint32_t ghc;     // Global host control
    uint32_t is;      // Interrupt status
    uint32_t pi;      // Port implemented
    uint32_t vs;      // Version
    uint32_t ccc_ctl; // Command completion coalescing control
    uint32_t ccc_pts; // Command completion coalescing ports
    uint32_t em_loc;  // Enclosure management location
    uint32_t em_ctl;  // Enclosure management control
    uint32_t cap2;    // Host capabilities extended
    uint32_t bohc;    // BIOS/OS handoff control and status
    uint8_t rsv[116];
    uint8_t vendor[96];
    hba_port_t port[0];
} __attribute__((packed)) hba_mem_t;

typedef volatile struct {
    uint8_t cfl : 5;   // Command FIS length in units of 4 bytes (2 - 16)
    uint8_t atapi : 1; // ATAPI
    uint8_t write : 1; // Write (1 = H2D, 0 = D2H)
    uint8_t pf : 1;    // Prefetchable
    uint8_t reset : 1; // Reset
    uint8_t bist : 1;  // BIST
    uint8_t clear : 1; // Clear busy upon R_OK
    uint8_t rsv0 : 1;  // Reserved
    uint8_t pmp : 4;   // Port multiplier port
    uint16_t prdtl;    // Physical region descriptor table length in entries
    uint32_t prdbc;    // Physical region descriptor count transferred
    uint64_t ctba;     // Command table descriptor base address (128 bytes aligned)
    uint32_t rsv1[4];  // Reserved
} __attribute__((packed)) hba_chdr_t;

typedef struct {
    uint64_t dba;      // Data base address
    uint32_t rsv0;     // Reserved
    uint32_t dbc : 22; // Byte count minus one
    uint32_t rsv1 : 9; // Reserved
    uint32_t ioc : 1;  // Interrupt on completion
} __attribute__((packed)) hba_prdt_t;

typedef struct {
    uint8_t cfis[64];   // Command FIS
    uint8_t acmd[16];   // ATAPI command
    uint8_t rsv[48];    // Reserved
    hba_prdt_t prdt[8]; // Physical region descriptor table entries (0 - 65535)
} __attribute__((packed)) hba_ctbl_t;

typedef struct {
    uint8_t type;       // FIS_TYPE_REG_H2D
    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 3;   // Reserved
    uint8_t c : 1;      // 1: Command, 0: Control
    uint8_t command;    // Command register
    uint8_t feature0;   // Feature register low
    uint32_t lba0 : 24; // LBA register low
    uint8_t device;     // Device register
    uint32_t lba1 : 24; // LBA register high
    uint8_t feature1;   // Feature register high
    uint16_t count;     // Count register
    uint8_t icc;        // Isochronous command completion
    uint8_t control;    // Control register
    uint32_t rsv1;      // Reserved
} __attribute__((packed)) fis_reg_h2d_t;

typedef struct ahci_dev ahci_dev_t;
typedef struct ahci_host ahci_host_t;

struct ahci_dev {
    uint8_t id;       // Port ID
    uint8_t atapi;    // ATAPI drive
    ata_info_t disk;  // ATA disk information
    ata_worker_t wk;  // ATA worker

    ahci_host_t *host; // AHCI host
    hba_port_t *port;  // ACHI port
    hba_chdr_t *clb;   // Command list base
    hba_ctbl_t *ctb;   // Command table base
    void *fb;          // FIS base

    struct {
        uint64_t phys;
        uint64_t virt;
        uint32_t size;
    } dma;

    volatile struct {
        uint8_t flag;
        uint8_t signal;
        uint32_t status;
        uint32_t type;
    } irq;
};

struct ahci_host {
    cap_t cap;
    uint8_t np;
    uint8_t ncs;
    hba_mem_t *hba;
    ahci_dev_t *dev;
};

void ahci_init(pci_dev_t*);
