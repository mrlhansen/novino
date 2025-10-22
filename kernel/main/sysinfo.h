#pragma once

#include <kernel/types.h>

enum {
    SI_MEMINFO  = 1,
    SI_PROCLIST = 2,
    SI_PROCINFO = 3,
    SI_CPULIST  = 4,
    SI_CPUINFO  = 5,
};

typedef struct {
    int cap;
    int size;
    char *data;
} sysinfo_t;

int sysinfo_write(sysinfo_t *sys, const char *fmt, ...);
int sysinfo(size_t req, size_t id, void *buf, size_t len);
