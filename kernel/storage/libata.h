#pragma once

#include <kernel/sched/kthreads.h>
#include <kernel/lists.h>
#include <kernel/types.h>

#define ATA_DMA_SIZE 524288

typedef struct {
    uint64_t sectors;  // Total number of logical sectors
    uint32_t pss;      // Physical sector size
    uint32_t lss;      // Logical sector size
    uint32_t flags;    // Feature flags
    uint8_t revision;  // ATA revision
    uint8_t atapi;     // ATAPI device (boolean)
    char serial[21];   // Serial string
    char firmware[9];  // Firmware string
    char model[41];    // Model string
} ata_info_t;

typedef struct ata_queue {
    void *dev;
    void *buf;
    int write;
    uint32_t count;
    uint64_t lba;
    thread_t *thread;
    volatile int status;
    link_t link;
} ata_queue_t;

typedef struct {
    int (*read)(void*, size_t, size_t, void*);
    int (*write)(void*, size_t, size_t, void*);
    thread_t *thread;
    list_t queue;
} ata_worker_t;

enum {
    ATAPI_TEST_UNIT_READY   = 0x00,
    ATAPI_REQUEST_SENSE     = 0x03,
    ATAPI_CMD_READ          = 0xA8,
    ATAPI_CMD_READ_CAPACITY = 0x25
};

enum {
    ATA_CMD_READ_PIO        = 0x20,
    ATA_CMD_READ_PIO_EXT    = 0x24,
    ATA_CMD_READ_DMA        = 0xC8,
    ATA_CMD_READ_DMA_EXT    = 0x25,
    ATA_CMD_WRITE_PIO       = 0x30,
    ATA_CMD_WRITE_PIO_EXT   = 0x34,
    ATA_CMD_WRITE_DMA       = 0xCA,
    ATA_CMD_WRITE_DMA_EXT   = 0x35,
    ATA_CMD_FLUSH_CACHE     = 0xE7,
    ATA_CMD_FLUSH_CACHE_EXT = 0xEA,
    ATA_CMD_PACKET          = 0xA0,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY        = 0xEC
};

enum {
    ATA_FLAG_DMA    = (1 << 0), // Supports DMA
    ATA_FLAG_UDMA   = (1 << 1), // Supports Ultra DMA
    ATA_FLAG_LBA28  = (1 << 2), // Supports 28-bit addressing
    ATA_FLAG_LBA48  = (1 << 3), // Supports 48-bit addressing
    ATA_FLAG_DMADIR = (1 << 4), // Direction bit required in packet command for DMA transfers
    ATA_FLAG_DRA    = (1 << 5), // Device Read-Ahead enabled
    ATA_FLAG_WCE    = (1 << 6), // Write Cache Enabled
};

void libata_identify(uint16_t*, ata_info_t*);
void libata_print(ata_info_t*, const char*, int, int);
int libata_queue(ata_worker_t*, void*, int, uint64_t, uint32_t, void*);
int libata_start_worker(ata_worker_t*, const char*);
