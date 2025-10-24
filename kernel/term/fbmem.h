#pragma once

#include <kernel/types.h>
#include <kernel/term/vts.h>

typedef struct  {
    uintptr_t addr;
    int width;
    int height;
    int pitch;
    int bpp;
} fbmem_t;

int fbmem_open();
int fbmem_close();
int fbmem_ioctl(fbmem_t *fb);
int fbmem_toggle(bool active);
void fbmem_init(vts_t *fbvts);
