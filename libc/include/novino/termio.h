#pragma once

// the following flags and structs must match the kernel ones

#define TIOECHO   (1 << 0) // Echo all
#define TIOECHONL (1 << 1) // Echo new line
#define TIONOBUF  (1 << 2) // Disable line buffering
#define TIOCURSOR (1 << 3) // Show cursor
#define TIOWRAP   (1 << 4) // Wrap horizontal cursor movements
#define TIONOSCRL (1 << 5) // Disable scrolling

typedef struct {
    unsigned int rows;
    unsigned int cols;
    unsigned int xpixel;
    unsigned int ypixel;
} tiowinsz_t;

char *tiogets(char *buf, int len);
int tiogetescape(const char *str, int *key, int *va, int *vb);
int tiosetflags(int flags);
int tiogetflags(int *flags);
int tiogetwinsz(tiowinsz_t *w);
