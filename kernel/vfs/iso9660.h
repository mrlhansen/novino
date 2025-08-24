#pragma once

#include <kernel/vfs/devfs.h>
#include <kernel/types.h>

enum {
    ISO9660_BRD = 0,   // Boot Record Descriptor
    ISO9660_PVD = 1,   // Primary Volume Descriptor
    ISO9660_SVD = 2,   // Supplementary Volume Descriptor
    ISO9660_VPD = 4,   // Volume Partition Descriptor
    ISO9660_VTD = 255, // Volume Terminate Descriptor
};

// Datetime format used in volume descriptor
typedef struct {
    char year[4];
    char month[2];
    char day[2];
    char hours[2];
    char minutes[2];
    char seconds[2];
    char hecoseconds[2];
    uint8_t tz_offset;
} iso9660_time;

// Primary (and supplementary) volume descriptor
typedef struct {
    uint8_t type;
    char id_standard[5];
    uint8_t version;
    uint8_t flags;
    char id_system[32];
    char id_volume[32];
    char unused[8];
    uint64_t space_size;
    char escape_sequence[32];
    uint32_t set_size;
    uint32_t sequence_number;
    uint32_t logical_block_size;
    uint64_t path_table_size;
    uint32_t l_path_table;
    uint32_t optional_l_path_table;
    uint32_t m_path_table;
    uint32_t optional_m_path_table;
    char root_record[34];
    char id_volume_set[128];
    char id_publisher[128];
    char id_data_prepare[128];
    char id_application[128];
    char id_copyright_file[37];
    char id_abstract_file[37];
    char id_bibliographic_file[37];
    iso9660_time dt_creation;
    iso9660_time dt_modification;
    iso9660_time dt_expiration;
    iso9660_time dt_effective;
    uint8_t file_structure_version;
    char padding[1166];
} __attribute__((packed)) iso9660_pvd_t;

// Directory entry
typedef struct {
    uint8_t length;
    uint8_t xattr_length;
    uint64_t extent; // LBA address
    uint64_t extent_size;
    struct {
        uint8_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint8_t gmt_offset;
    } dt;
    uint8_t flags;
    uint8_t file_unit_size;
    uint8_t gap_size;
    uint32_t volume_sequence_number;
    uint8_t filename_length;
    char filename[256];
} __attribute__((packed)) iso9660_dirent_t;

// Filesystem data
typedef struct {
    uint32_t root_location;
    uint32_t root_length;
    uint32_t bps;
    uint32_t sectors;
    uint8_t  joliet_level;
    devfs_t *dev;
} iso9660_t;

void iso9660_init();
