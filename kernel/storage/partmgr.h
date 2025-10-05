#pragma once

#include <kernel/vfs/types.h>

typedef struct {
    uint32_t flags     : 8;
    uint32_t chs_start : 24;
    uint32_t type      : 8;
    uint32_t chs_last  : 24;
    uint32_t lba_start;
    uint32_t lba_size;
} __attribute__((packed)) mbr_entry_t;

typedef struct {
    uint64_t signature;   // Signature (EFI PART)
    uint32_t revision;    // Revision (1.0)
    uint32_t size;        // Header size in bytes
    uint32_t crc32;       // CRC32 checksum
    uint32_t reserved;
    uint64_t lba_current; // LBA of this header
    uint64_t lba_backup;  // LBA of backup header
    uint64_t lba_first;   // First usable LBA for partitions
    uint64_t lba_last;    // Last usable LBA for partitions
    uint8_t guid[16];     // GUID of drive
    uint64_t pa_lba;      // LBA address of partition array
    uint32_t pa_count;    // Number of entries in partition array
    uint32_t pa_size;     // Size of each entry in partition array
    uint32_t pa_crc32;    // CRC32 checksum of partition array
} __attribute__((packed)) gpt_header_t;

typedef struct {
    uint8_t guid_type[16];
    uint8_t guid_unique[16];
    uint64_t lba_first;
    uint64_t lba_last;
    uint64_t flags;
    uint8_t name[72];
} __attribute__((packed)) gpt_entry_t;

int partmgr_init(devfs_t *dev);
