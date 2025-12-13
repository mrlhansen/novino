#pragma once

#include <kernel/atomic.h>
#include <kernel/lists.h>
#include <kernel/types.h>

#define MAX_SFN 16
#define MAX_LFN 256

// File context flags
enum {
    O_READ     = 0x01, // Reading
    O_WRITE    = 0x02, // Writing
    O_TRUNC    = 0x04, // Truncate file on open
    O_APPEND   = 0x08, // Seek to end of file on open
    O_DIR      = 0x10, // Open a directory
    O_CREATE   = 0x20, // Create file on open
    O_NONBLOCK = 0x40, // Non-blocking
};

// Inode flags
enum {
    I_FILE     = 0x01, // Regular file
    I_DIR      = 0x02, // Directory
    I_BLOCK    = 0x04, // Block device
    I_STREAM   = 0x08, // Stream device
    I_SYMLINK  = 0x10, // Symbolic link
    I_PIPE     = 0x20, // Anonymous pipe
};

// Seeking
#define SEEK_SET 0 // Seek relative to beginning
#define SEEK_CUR 1 // Seek relative to current position
#define SEEK_END 2 // Seek relative to end

// Type definitions
typedef struct blkdev blkdev_t;
typedef struct devfs devfs_t;
typedef struct vfs_fs vfs_fs_t;
typedef struct vfs_mp vfs_mp_t;
typedef struct vfs_ops vfs_ops_t;
typedef struct inode inode_t;
typedef struct dentry dentry_t;

// File inode
struct inode {
    uint64_t ino;    // Inode number
    uint16_t flags;  // Inode flags
    uint16_t mode;   // Permissions
    uint32_t links;  // Number of hard links
    uint32_t uid;    // User ID
    uint32_t gid;    // Group ID
    uint64_t size;   // Size in bytes
    uint64_t blocks; // Number of blocks
    uint64_t blksz;  // Block size
    uint64_t atime;  // Time of last access
    uint64_t mtime;  // Time of last modification
    uint64_t ctime;  // Time of last status change

    vfs_ops_t *ops;  // Filesystem operations
    void *data;      // Private filesystem data
    void *obj;       // Pointer to object (for memory backed filesystems)
};

// Directory entry (dcache)
struct dentry {
    char name[MAX_LFN];  // Entry name
    bool cached;         // All child entries are cached
    size_t hash;         // Hash of entry name
    size_t positive;     // Number of valid entries
    size_t negative;     // Number of negative entries
    inode_t *inode;      // Associated inode (zero for negative entries)
    atomic_t numfd;      // Number of file context references
    vfs_mp_t *mp;        // Mountpoint

    dentry_t *parent;    // Parent entry
    dentry_t *child;     // Child entries
    dentry_t *next;      // Next entry
};

// Directory entry (short)
typedef struct {
    uint16_t length;  // Total length
    uint16_t flags;   // Copy of the inode flags
    char name[];      // Name of the directory entry
} dirent_t;

// File stat
typedef struct {
    uint64_t ino;    // Inode number
    uint16_t flags;  // Inode flags
    uint16_t mode;   // Permissions
    uint32_t links;  // Number of hard links
    uint32_t uid;    // User ID
    uint32_t gid;    // Group ID
    uint64_t size;   // Size in bytes
    uint64_t blocks; // Number of blocks
    uint64_t blksz;  // Block size
    uint64_t atime;  // Time of last access
    uint64_t mtime;  // Time of last modification
    uint64_t ctime;  // Time of last status change
} stat_t;

// Context for an open file
typedef struct file {
    atomic_t refs;     // Number of file descriptor references
    size_t flags;      // Flags
    size_t seek;       // Current position
    dentry_t *dentry;  // Directory entry for the file
    inode_t *inode;    // Inode for the file
    void *data;        // Private filesystem data
} file_t;

// Filesystem operations
typedef struct vfs_ops {
    int (*open)(file_t*);
    int (*close)(file_t*);
    int (*read)(file_t*, size_t, void*);
    int (*write)(file_t*, size_t, void*);
    int (*seek)(file_t*, ssize_t, int);
    int (*ioctl)(file_t*, size_t, size_t);
    int (*readdir)(file_t*, size_t, void*);
    int (*lookup)(inode_t*, const char*, inode_t*);
    int (*truncate)(inode_t*);
    int (*setattr)(inode_t*);
    int (*getattr)(inode_t*);
    int (*create)(dentry_t*);
    int (*remove)(dentry_t*);
    int (*rename)(dentry_t*, dentry_t*);
    int (*mkdir)(dentry_t*);
    int (*rmdir)(dentry_t*);
    void* (*mount)(devfs_t*, inode_t*);
    int (*umount)(void*);
};

// Stream device operations
typedef struct {
    int (*open)(file_t*);
    int (*close)(file_t*);
    int (*read)(file_t*, size_t, void*);
    int (*write)(file_t*, size_t, void*);
    int (*seek)(file_t*, ssize_t, int);
    int (*ioctl)(file_t*, size_t, size_t);
} devfs_ops_t;

// Block device operations
typedef struct {
    int (*status)(void*, blkdev_t*, int);
    int (*read)(void*, size_t, size_t, void*);
    int (*write)(void*, size_t, size_t, void*);
} devfs_blk_t;

// Filesystem
struct vfs_fs {
    char name[MAX_SFN]; // Name of filesystem
    uint32_t flags;     // Filesystem flags
    vfs_ops_t *ops;     // Operations
    link_t link;        // Link for list of filesystems
};

// Mountpoint
struct vfs_mp {
    char name[MAX_SFN]; // Name of mountpoint
    vfs_fs_t *fs;       // Link to filesystem
    devfs_t *dev;       // Link to device
    inode_t inode;      // Root inode
    dentry_t dentry;    // Root dentry
    atomic_t numfd;     // Number of open files for this mountpoint
    link_t link;        // Link for list of mountpoints
};

// Device file
typedef struct devfs {
    char name[MAX_SFN];    // Name of device
    inode_t inode;         // Device inode
    blkdev_t *bd;          // Block device descriptor
    union {
        devfs_ops_t *ops;  // Stream operations
        devfs_blk_t *blk;  // Block operations
    };
    void *data;            // Private device data
    devfs_t *next;         // Link to next
    devfs_t *child;        // Link to children
    devfs_t *parent;       // Link to parent
};

// Internal struct for VFS path iteration
typedef struct {
    char *prev;
    char *curr;
    char *pos;
    int depth;
    dentry_t *root;
    char path[];
} vfs_path_t;

// Internal struct for readdir
typedef struct {
    file_t *file;     // File context
    dentry_t *parent; // Parent directory entry
    ssize_t status;   // Propagated return status
    void *dirent;     // Buffer for entries
    size_t size;      // Buffer size
    size_t pos;       // Buffer position
} vfs_rd_t;
