#pragma once

#include <kernel/vfs/devfs.h>
#include <kernel/atomic.h>
#include <kernel/lists.h>

// ext2 superblock
typedef struct {
    uint32_t inodes_count;           // Total number of inodes
    uint32_t blocks_count;           // Total number of blocks
    uint32_t reserved_blocks_count;  // Number of blocks reserved for superuser
    uint32_t free_blocks_count;      // Total number of unallocated blocks
    uint32_t free_inodes_count;      // Total number of unallocated inodes
    uint32_t first_data_block;       // Block number of the block containing the superblock
    uint32_t log_block_size;         // Block size calculated as (1024 << N)
    uint32_t log_frag_size;          // Fragment size calculated as (1024 << N)
    uint32_t blocks_per_group;       // Number of blocks in each block group
    uint32_t frags_per_group;        // Number of fragments in each block group
    uint32_t inodes_per_group;       // Number of inodes in each block group
    uint32_t mount_time;             // Last mount time
    uint32_t write_time;             // Last written time
    uint16_t mount_count;            // Number of times the volume has been mounted since its last consistency check
    uint16_t max_mount_count;        // Number of mounts allowed before a consistency check must be done
    uint16_t signature;              // EXT2 signature (0xEF53)
    uint16_t state;                  // File system state
    uint16_t errors;                 // What to do when an error is detected
    uint16_t minor_revision;         // EXT2 minor version
    uint32_t check_time;             // Time of last consistency check
    uint32_t check_interval;         // Interval between forced consistency checks
    uint32_t creator_os;             // Operating system ID from which the filesystem was created
    uint32_t major_revision;         // EXT2 major version
    uint16_t def_resuid;             // User ID that can use reserved blocks
    uint16_t def_resgid;             // Group ID that can use reserved blocks
    uint32_t first_inode;            // First non-reserved inode in file system.
    uint16_t inode_size;             // Size of each inode structure in bytes
    uint16_t block_group_number;     // Block group that this superblock is part of (for backup copies)
    uint32_t features_optional;      // Optional features presen
    uint32_t features_required;      // Required features present
    uint32_t features_readonly;      // Features that if not supported, the volume must be mounted read-only
    uint8_t uuid[16];                // File system ID
    char volume_name[16];            // Volume name (null terminated)
    char last_mounted[64];           // Last path the volume was mounted on (null terminated)
    uint32_t algorithm_usage_bitmap; // Compression algorithms used
    uint8_t prealloc_file_blocks;    // Number of blocks to preallocate for files
    uint8_t prealloc_dir_blocks;     // Number of blocks to preallocate for directories
    uint16_t reserved;               // Reserved
    uint8_t journal_uuid[16];        // Journal ID
    uint32_t journal_inode;          // Journal inode
    uint32_t journal_device;         // Journal device
    uint32_t last_orphan;            // Head of orphan inode list
} __attribute__((packed)) ext2_sb_t;

// ext2 inode
typedef struct {
    uint16_t mode;         // Type and permissions
    uint16_t uid;          // User ID
    uint32_t size;         // Size in bytes (lower bits)
    uint32_t atime;        // Last access time
    uint32_t ctime;        // Creation time
    uint32_t mtime;        // Last modification time
    uint32_t dtime;        // Deletion time
    uint16_t gid;          // Group ID
    uint16_t links;        // Number of hard links to this inode
    uint32_t sectors;      // Number of sectors used by this inode
    uint32_t flags;        // Inode flags
    uint32_t osval;        // Operating system specific value #1
    uint32_t block[15];    // Block pointers
    uint32_t generation;   // Generation number
    uint32_t file_acl;     // Extended attribute block (File ACL)
    union {
        uint32_t dir_acl;  // Directory ACL
        uint32_t size_ext; // Size in bytes (upper bits)
    };
    uint32_t fragment;     // Block address of fragment
    uint32_t reserved[3];  // Operating system specific value #2
} __attribute__((packed)) ext2_inode_t;

// ext2 block group descriptor
typedef struct {
    uint32_t block_bitmap; // Block address of block usage bitmap
    uint32_t inode_bitmap; // Block address of inode usage bitmap
    uint32_t inode_table;  // Starting block address of inode table
    uint16_t free_blocks;  // Number of unallocated blocks in group
    uint16_t free_inodes;  // Number of unallocated inodes in group
    uint16_t used_dirs;    // Number of directories in group
    uint8_t reserved[14];  // Reserved
} __attribute__((packed)) ext2_bgd_t;

// ext2 directory entry
typedef struct {
    uint32_t inode;         // Inode number
    uint16_t size;          // Total size of this entry
    uint8_t length;         // Name length (lower bits)
    union {
        uint8_t type;       // Type indicator (requires feature bit)
        uint8_t length_ext; // Name of length (upper bits)
    };
    char name[];            // File name
} __attribute__((packed)) ext2_dentry_t;

typedef struct {
    uint32_t block_size;         // Block size in bytes
    uint32_t inodes_per_block;   // Inodes per block
    uint16_t sectors_per_block;  // Sectors per block
    uint32_t block_groups_count; // Number of block groups
    uint32_t bgds_per_block;     // Block group descriptors per block
    uint32_t bgds_start;         // First block group descriptor
    uint8_t version;             // File system version (ext2,3,4)
    ext2_sb_t *sb;               // Superblock
    devfs_t *dev;                // Block device
    void *ctx;                   // Default context
} ext2_t;

typedef struct {
    uint32_t block; // Block number
    bool dirty;     // Block has been modified
    link_t link;    // Link to next block
    void *data;     // Block data
} ext2_blk_t;

typedef struct {
    ext2_t *fs;          // Context filesystem
    lock_t lock;         // Context lock
    list_t list;         // List of cached blocks
    int errno;           // Propagated error code
    size_t ptr_links[3]; // Blocks used for indirect pointers
    size_t ptr_ident;    // Cached lookup in ext2_inode_block
    size_t ptr_block;    // Cached lookup in ext2_inode_block
    void *ptr_data;      // Cached lookup in ext2_inode_block
    void *blkbuf;        // Generic block buffer
} ext2_ctx_t;

void ext2_init();
