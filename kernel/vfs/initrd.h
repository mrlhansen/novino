#pragma once

#include <kernel/types.h>

typedef struct {
    char name[100];      // File name
    char mode[8];        // File mode (octal)
    char uid[8];         // Owner user ID (octal)
    char gid[8];         // Owner group ID (octal)
    char size[12];       // File size (octal)
    char mtime[12];      // Unix timestamp for last modification (octal)
    char checksum[8];    // Checksum
    char typeflag;       // Type flag
    char linkname[100];  // Name of linked file
    char magic[6];       // Magic value
    char version[2];     // Version
    char uname[32];      // Owner user name
    char gname[32];      // Owner group name
    char devmajor[8];    // Device major number
    char devminor[8];    // Device major number
    char prefix[145];    // Filename prefix
    // the struct is normally 500 bytes, but we need 20 bytes for the entries below
    // so we steal a bit of space from the prefix (normally 155 bytes)
    // by assuming we do not have paths above 245 bytes
    uint32_t isize;      // File size (bytes)
    uint32_t offset;     // Offset to next entry
    uint32_t depth;      // Path depth
    char *basename;      // Basename
} __attribute__((packed)) ustar_t;

#define TMAGIC   "ustar"
#define TMAGLEN  6
#define TVERSION "00"
#define TVERSLEN 2

#define REGTYPE  '0'     // regular file
#define LNKTYPE  '1'     // hard link
#define SYMTYPE  '2'     // symbolic link
#define CHRTYPE  '3'     // character special
#define BLKTYPE  '4'     // block special
#define DIRTYPE  '5'     // directory
#define FIFOTYPE '6'     // named pipe (fifo)

#define TSUID    04000   // set UID on execution
#define TSGID    02000   // set GID on execution
#define TSVTX    01000   // reserved

#define TUREAD   00400   // read by owner
#define TUWRITE  00200   // write by owner
#define TUEXEC   00100   // execute/search by owner
#define TGREAD   00040   // read by group
#define TGWRITE  00020   // write by group
#define TGEXEC   00010   // execute/search by group
#define TOREAD   00004   // read by other
#define TOWRITE  00002   // write by other
#define TOEXEC   00001   // execute/search by other

void initrd_init(uint64_t, uint32_t);
