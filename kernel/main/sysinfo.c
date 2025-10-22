#include <kernel/sched/process.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/vmm.h>
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

static void sysinfo_meminfo(sysinfo_t *sys)
{
    size_t total, free;

    total = PAGE_SIZE * pmm_usable_pages();
    free  = PAGE_SIZE * pmm_free_pages();

    sysinfo_write(sys, "total=%lu", total);
    sysinfo_write(sys, "free=%lu", free);
    sysinfo_write(sys, "heap=%lu", heap_get_size());
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
            sysinfo_meminfo(&sys);
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
