#include <kernel/sched/process.h>
#include <kernel/sysinfo.h>
#include <kernel/errno.h>
#include <string.h>
#include <stdio.h>

int sysinfo_write(sysinfo_t *sys, const char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;
    int rem;

    rem = sys->cap - sys->size;
    if(!rem)
    {
        return -ENOSPC;
    }

    if(sys->size)
    {
        sys->data[sys->size] = ';';
        sys->size++;
        rem--;
    }

    va_start(args, fmt);
    len = vsprintf(buf, fmt, args); // vsnprintf here instead
    va_end(args);

    if(len > rem)
    {
        len = rem;
    }

    strncpy(sys->data + sys->size, buf, len);
    sys->size += len;
    sys->data[sys->size] = '\0';

    return 0;
}

int sysinfo(size_t req, size_t id, void *buf, size_t len)
{
    sysinfo_t sys = {
        .cap = len - 1,
        .size = 0,
        .data = buf,
    };

    switch(req)
    {
        case SI_MEMINFO:
            break;
        case SI_PROCLIST:
            sysinfo_proclist(&sys);
            break;
        case SI_PROCINFO:
            sysinfo_procinfo(&sys, id);
            break;
        case SI_CPULIST:
            break;
        case SI_CPUINFO:
            break;
        default:
            break;
    }

    sys.data[sys.size] = '\0';
    return sys.size + 1;
}
