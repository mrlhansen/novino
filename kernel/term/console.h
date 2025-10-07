#pragma once

#include <kernel/sched/spinlock.h>
#include <loader/bootstruct.h>
#include <kernel/types.h>

#define VGA_BLACK          0
#define VGA_BLUE           1
#define VGA_GREEN          2
#define VGA_CYAN           3
#define VGA_RED            4
#define VGA_MAGENTA        5
#define VGA_BROWN          6
#define VGA_WHITE          7
#define VGA_GRAY           8
#define VGA_BRIGHT_BLUE    9
#define VGA_BRIGHT_GREEN   10
#define VGA_BRIGHT_CYAN    11
#define VGA_BRIGHT_RED     12
#define VGA_BRIGHT_MAGENTA 13
#define VGA_YELLOW         14
#define VGA_BRIGHT_WHITE   15

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t nbytes;
    uint8_t data[4];
} color_t;

typedef struct {
    uint32_t ch;
    color_t fg;
    color_t bg;
} glyph_t;

typedef struct {
    // vesa mode
    uint64_t lfbphys;
    uint64_t lfbaddr;
    uint64_t bufaddr;
    uint32_t bytes_per_line;
    uint32_t bytes_per_pixel;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    glyph_t *glyphs;
    color_t fg;
    color_t bg;

    // text mode
    uint16_t *memptr;
    uint16_t *bufptr;
    uint8_t attribute;

    // generic
    spinlock_t lock;
    uint32_t active;
    uint32_t flags;
    uint32_t memsz;
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t max_x;
    uint32_t max_y;
} console_t;

// vga.c
void vga_refresh(console_t*);
void vga_scroll(console_t*);
void vga_clear(console_t*);
void vga_toggle_cursor(console_t*);
void vga_set_color(console_t*, int, int);
void vga_putch(console_t*, int);
void vga_init(console_t*);

// vesa.c
void vesa_refresh(console_t*);
void vesa_scroll(console_t*);
void vesa_clear(console_t*);
void vesa_toggle_cursor(console_t*);
void vesa_set_color(console_t*, int, int);
void vesa_putch(console_t*, int);
void vesa_init(console_t*, bootstruct_t*);
int vesa_reinit(console_t*);

// console.c
void console_clear(console_t*);
void console_refresh(console_t*);
void console_set_flags(console_t*, int);
void console_move_cursor(console_t*, int, int, bool);
void console_set_color(console_t*, int, int);
void console_putch(console_t*, int);
void console_puts(console_t*, char*);
void console_init(bootstruct_t*);

console_t *console_clone();
console_t *console_default();
