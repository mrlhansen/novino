#pragma once

enum {
    EOK,          // No error
    ENOSYS,       // Invalid system call
    EPERM,        // Operation not permitted
    EBUSY,        // Resource is busy
    ENXIO,        // No such device or address
    ENOENT,       // No such file or directory
    ENOTDIR,      // Not a directory
    EISDIR,       // Is a directory
    ENOTEMPTY,    // Directory is not empty
    ENODEV,       // No such device
    ENOFS,        // No such file system
    ENOSPC,       // No space left
    EDOM,         // Argument out of domain
    ERANGE,       // Argument out of range
    EILSEQ,       // Illegal byte sequence
    EROFS,        // Read-only file system
    EBADF,        // Bad file descriptor
    ENOIOCTL,     // No such ioctl command
    ENAMETOOLONG, // Filename too long
    ENOEXEC,      // Invalid executable format
    EFAIL,        // Operation failed
    ENOMEM,       // Out of memory
    EINVAL,       // Invalid argument
    ETMOUT,       // Timeout
    EIO,          // IO error
    ENOTSUP,      // Not supported
    ENOINT,       // No interrupt received
    EEXIST,       // Object already exists
    ENOMEDIUM,    // No medium present
    ENOACK,       // No ACK from device
    EPIPE,        // Broken pipe
    EFAULT,       // Bad address
};
