#include <string.h>
#include <stdio.h>
#include <errno.h>

char *strerror(int errnum)
{
    static char buf[32];
    char *str = 0;

    if(errnum < 0)
    {
        errnum = -errnum;
    }

    switch(errnum)
    {
        case ENOSYS:
            str = "Invalid system call";
            break;
        case EPERM:
            str = "Operation not permitted";
            break;
        case EBUSY:
            str = "Resource is busy";
            break;
        case ENXIO:
            str = "No such device or address";
            break;
        case ENOENT:
            str = "No such file or directory";
            break;
        case ENOTDIR:
            str = "Not a directory";
            break;
        case EISDIR:
            str = "Is a directory";
            break;
        case ENODEV:
            str = "No such device";
            break;
        case ENOFS:
            str = "No such file system";
            break;
        case ENOSPC:
            str = "No space left";
            break;
        case EDOM:
            str = "Argument out of domain";
            break;
        case ERANGE:
            str = "Argument out of range";
            break;
        case EILSEQ:
            str = "Illegal byte sequence";
            break;
        case EROFS:
            str = "Read-only file system";
            break;
        case EBADF:
            str = "Bad file descriptor";
            break;
        case ENOIOCTL:
            str = "No such ioctl command";
            break;
        case ENAMETOOLONG:
            str = "Filename too long";
            break;
        case ENOEXEC:
            str = "Invalid executable format";
            break;
        case EFAIL:
            str = "Operation failed";
            break;
        case ENOMEM:
            str = "Out of memory";
            break;
        case EINVAL:
            str = "Invalid argument";
            break;
        case ETMOUT:
            str = "Timeout";
            break;
        case EIO:
            str = "IO error";
            break;
        case ENOTSUP:
            str = "Not supported";
            break;
        case ENOINT:
            str = "No interrupt received";
            break;
        case EEXIST:
            str = "Object already exists";
            break;
        case ENOMEDIUM:
            str = "No medium present";
            break;
        case ENOACK:
            str = "No ACK from device";
            break;
        case EPIPE:
            str = "Broken pipe";
            break;
        case EFAULT:
            str = "Bad address";
            break;
        default:
            break;
    };

    if(!str)
    {
        sprintf(buf, "Unknown error %d", errnum);
        str = buf;
    }

    return str;
}
