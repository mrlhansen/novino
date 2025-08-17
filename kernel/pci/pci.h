#pragma once

#include <kernel/lists.h>
#include <kernel/types.h>

enum {
    CONFIG_ADDR = 0xCF8,
    CONFIG_DATA = 0xCFC
};

enum {
    BAR_ZERO  = 0x00,
    BAR_PMIO  = 0x01,
    BAR_MMIO  = 0x02,
    BAR_16BIT = 0x10,
    BAR_32BIT = 0x20,
    BAR_64BIT = 0x40
};

enum {
    PCI_VENDOR_ID      = 0x00,
    PCI_DEVICE_ID      = 0x02,
    PCI_COMMAND        = 0x04,
    PCI_STATUS         = 0x06,
    PCI_REVISION       = 0x08,
    PCI_PROG_IF        = 0x09,
    PCI_SUBCLASS       = 0x0A,
    PCI_CLASSCODE      = 0x0B,
    PCI_HEADER_TYPE    = 0x0E,
    PCI_SECONDARY_BUS  = 0x19,
    PCI_CAP_OFFSET     = 0x34,
    PCI_INTERRUPT_LINE = 0x3C,
    PCI_INTERRUPT_PIN  = 0x3D,
};

enum {
    PCI_IRQ_ENABLED = (1 << 0),
    PCI_IRQ_GSI     = (1 << 1),
    PCI_IRQ_MSI     = (1 << 2),
    PCI_IRQ_MSIX    = (1 << 3),
};

typedef struct {
    uint8_t vector;
    uint8_t flags;
    uint16_t index;
} pci_irq_t;

typedef struct {
    int offset;     // Offset for capability structure (zero when unsupported)
    int size;       // Max number of vectors
    int cap_64bit;  // 64-bit address capable
    int cap_pvm;    // Per-vector masking capable
    int cap_mm;     // Multi-message capable
} pci_msi_t;

typedef struct {
    int offset;     // Offset for capability structure (zero when unsupported)
    int size;       // Max number of vectors
    size_t table;   // Vector table
    size_t pba;     // Pending Bit Array (PBA)
} pci_msix_t;

typedef struct {
    uint16_t vendor;
    uint16_t device;
    uint8_t revision;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t classcode;
    uint32_t pmio;
    uint64_t mmio;
    int gsi;            // GSI for legacy interrupts (-1 when absent)
    pci_msi_t msi;      // MSI support
    pci_msix_t msix;    // MSI-X support
    pci_irq_t *vecs;    // Allocated IRQ vectors
    uint16_t numvecs;   // Number of allocated IRQ vectors
} pci_dev_t;

typedef struct {
    uint8_t type;
    uint64_t value;
    uint64_t size;
} pci_bar_t;

typedef struct {
    pci_dev_t dev;    // PCI device structure
    uint8_t seg;      // Segment
    uint8_t bus;      // Bus
    uint8_t slot;     // Slot
    uint8_t fn;       // Function
    uint8_t pin;      // Interrupt pin
    uint8_t bridge;   // Bridge device (boolean)
    uint8_t br_sbus;  // Bridge: Secondary Bus
    void *br_prt;     // Bridge: PCI Routing Table (from ACPI)
    int br_prt_cnt;   // Bridge: Number of PRT elements
    link_t link;      // Link to next
} pci_dev_list_t;

typedef struct {
    uint16_t first;
    uint16_t second;
    void (*init)(pci_dev_t*);
} pci_drv_list_t;

pci_dev_list_t *pci_next_device(pci_dev_list_t*);

int pci_alloc_irq_vectors(pci_dev_t *dev, size_t minvecs, size_t maxvecs);
void pci_free_irq_vectors(pci_dev_t *dev);
int pci_irq_vector(pci_dev_t *dev, size_t num);

void pci_read_byte(pci_dev_t*, int, uint8_t*);
void pci_read_word(pci_dev_t*, int, uint16_t*);
void pci_read_dword(pci_dev_t*, int, uint32_t*);
void pci_write_byte(pci_dev_t*, int, uint8_t);
void pci_write_word(pci_dev_t*, int, uint16_t);
void pci_write_dword(pci_dev_t*, int, uint32_t);

void pci_enable_bm(pci_dev_t*);
void pci_disable_bm(pci_dev_t*);
void pci_enable_int(pci_dev_t*);
void pci_disable_int(pci_dev_t*);
void pci_read_bar(pci_dev_t*, int, pci_bar_t*);

void pci_find_driver(pci_dev_t*);
void pci_route_init();
void pci_init();
