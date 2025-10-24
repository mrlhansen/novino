#pragma once

#include <kernel/term/vts.h>

enum {
    ECHO   = (1 << 0), // Echo all
    ECHONL = (1 << 1), // Echo new line
    NOBUF  = (1 << 2), // Disable line buffering
    CURSOR = (1 << 3), // Show cursor
    WRAP   = (1 << 4), // Wrap horizontal cursor movements
    NOSCRL = (1 << 5), // Disable scrolling
};

enum {
    TIOGETFLAGS = 0x1432fa, // Get flags
    TIOSETFLAGS = 0x124039, // Set flags
    TIOGETWINSZ = 0x174564, // Get window size
    TIOMAPFBMEM = 0x51646e, // Map framebuffer
};

typedef struct {
    uint32_t rows;
    uint32_t cols;
    uint32_t xpixel;
    uint32_t ypixel;
} winsz_t;

void term_stdout(vts_t *vts, char *seq, size_t len);
void term_switch(int num);
void term_init();
