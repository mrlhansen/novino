#pragma once

#include <kernel/vfs/devfs.h>
#include <kernel/types.h>

// ext2 superblock
typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t signature;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_revision;
    uint32_t check_time;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t major_revision;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t block_group_number;
    uint32_t features_optional;
    uint32_t features_required;
    uint32_t features_readonly;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algorithm_usage_bitmap;
    uint8_t prealloc_file_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t padding;
    uint8_t journal_uuid[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t last_orphan;
    uint32_t hash_seed[4];
    uint8_t def_hash_version;
    uint8_t reserved_char_pad;
    uint16_t reserved_word_pad;
    uint32_t default_mount_opts;
    uint32_t first_meta_bg;
} __attribute__((packed)) ext2_sb_t;

// ext2 inode
typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t sectors_count;
    uint32_t flags;
    uint32_t osval;
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t fragment;
    uint32_t reserved[3];
} __attribute__((packed)) ext2_inode_t;

// ext2 block group descriptor
typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint8_t reserved[14];
} __attribute__((packed)) ext2_bgd_t;

// ext2 directory entry
typedef struct {
    uint32_t inode;
    uint16_t size;
    uint8_t length;
    uint8_t type;
    char name[0];
} __attribute__((packed)) ext2_dentry_t;

typedef struct {
    uint32_t block_size;
    uint32_t inodes_per_block;
    uint16_t sectors_per_block;
    uint32_t block_groups_count;
    uint32_t bgds_per_block;
    uint32_t bgds_start;
    uint8_t version;
    ext2_sb_t *sb;
    devfs_t *dev;
    int errno;
} ext2_t;

void ext2_init();
