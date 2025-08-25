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

typedef union {
    struct {
        uint16_t type;
        uint8_t length;
    };
    struct {
        uint16_t type;
        uint8_t length;
        uint8_t version;
        uint32_t mode;
        uint32_t : 32;
        uint32_t links;
        uint32_t : 32;
        uint32_t uid;
        uint32_t : 32;
        uint32_t gid;
        uint32_t : 32;
    } __attribute__((packed)) px;
    struct {
        uint16_t type;
        uint8_t length;
        uint8_t version;
        uint8_t flags;
        char name[];
    } __attribute__((packed)) nm;
} iso9660_susp_t;

// Datetime format used in volume descriptor
typedef struct {
    char year[4];        // Year (1-9999)
    char month[2];       // Month (1-12)
    char day[2];         // Day (1-31)
    char hours[2];       // Hours (0-23)
    char minutes[2];     // Minutes (0-59)
    char seconds[2];     // Seconds (0-59)
    char centisecs[2];   // Hundredths of a second (0-99)
    uint8_t gmt_offset;  // Offset from GMT in 15 minute intervals
} __attribute__((packed)) iso9660_time_t;

// Primary (and supplementary) volume descriptor
typedef struct {
    uint8_t type;                   // Descriptor type
    char id_standard[5];            // Standard identifier (always CD001)
    uint8_t version;                // Version (always 1)
    uint8_t reserved;
    char id_system[32];             // System identifier
    char id_volume[32];             // Volume identifier
    char unused[8];
    uint32_t space_size;            // Number of logical blocks in the volume
    uint32_t : 32;
    char escape_sequence[32];       // Escape sequence for SVD, reserved for PVD
    uint16_t set_size;              // Number of disks in the volume set
    uint16_t : 16;
    uint16_t sequence_number;       // Disk number in volume set
    uint16_t : 16;
    uint16_t logical_block_size;    // Bytes per sector (almost always 2048)
    uint16_t : 16;
    uint32_t path_table_size;       // Size of the path table
    uint32_t : 32;
    uint32_t l_path_table;          // Path table in little endian format
    uint32_t optional_l_path_table; // Optional path table in little endian format
    uint32_t m_path_table;          // Path table in big endian format
    uint32_t optional_m_path_table; // Optional path table in big endian format
    char root_record[34];           // Directory entry for the root directory
    char id_volume_set[128];        // Volume set identifier
    char id_publisher[128];         // Publisher identifier
    char id_data_prepare[128];      // Data preparer identifier
    char id_application[128];       // Application identifier
    char id_copyright_file[37];     // Filename of copyright file in the root directory
    char id_abstract_file[37];      // Filename of abstract file in the root directory
    char id_bibliographic_file[37]; // Filename of bibliographic file in the root directory
    iso9660_time_t dt_creation;     // Volume creation date and time
    iso9660_time_t dt_modification; // Volume modification date and time
    iso9660_time_t dt_expiration;   // Volume expiration date and time (when the disk is considered obsolete)
    iso9660_time_t dt_effective;    // Volume effective date and time (from when the disk may be used)
    uint8_t file_structure_version; // The directory records and path table version (always 1)
    char padding[1166];
} __attribute__((packed)) iso9660_pvd_t;

// Directory entry
typedef struct {
    uint8_t length;          // Length of directory record
    uint8_t xattr_length;    // Extended attribute record length
    uint32_t extent;         // Location of extent (LBA address)
    uint32_t : 32;
    uint32_t extent_size;    // Size of extent (in bytes)
    uint32_t : 32;
    struct {
        uint8_t year;        // Year since 1900
        uint8_t month;       // Month (1-12)
        uint8_t day;         // Day (1-31)
        uint8_t hour;        // Hours (0-23)
        uint8_t minute;      // Minutes (0-59)
        uint8_t second;      // Seconds (0-59)
        uint8_t gmt_offset;  // Offset from GMT in 15 minute intervals
    } dt;
    uint8_t flags;           // File flags
    uint8_t file_unit_size;  // File unit size (for interleaved mode)
    uint8_t gap_size;        // Interleave gap size
    uint16_t vol_seq_num;    // Volume sequence number
    uint16_t : 16;
    uint8_t filename_length; // Length of filename
    char filename[];         // Filename
} __attribute__((packed)) iso9660_dirent_t;

// Filesystem data
typedef struct {
    uint32_t root_extent;  // LBA address of the root directory
    uint32_t root_size;    // Size of the root directory (in bytes)
    uint32_t bps;          // Bytes per sector
    uint32_t sectors;      // Total number of sectors
    uint8_t joliet_level;  // Joilet level (UCS-2)
    devfs_t *dev;          // Mounted device
} iso9660_t;

void iso9660_init();
