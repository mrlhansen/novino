#include <kernel/term/console.h>
#include <kernel/x86/ioports.h>
#include <kernel/mem/heap.h>

static inline void memcpyw(uint16_t *dst, uint16_t *src, int len)
{
    while(len--)
    {
        *dst++ = *src++;
    }
}

static inline void memsetw(uint16_t *dst, uint16_t val, int len)
{
    while(len--)
    {
        *dst++ = val;
    }
}

void vga_refresh(console_t *con)
{
    uint16_t size;
    if(con->active)
    {
        size = con->max_x * con->max_y;
        memcpyw(con->memptr, con->bufptr, size);
    }
}

void vga_scroll(console_t *con)
{
    uint16_t blank, size;

    blank = (con->attribute << 8) | 0x20;
    size = con->max_x * (con->max_y - 1);
    memcpyw(con->bufptr, con->bufptr + con->max_x, size);
    memsetw(con->bufptr + size, blank, con->max_x);

    if(con->active)
    {
        size = con->max_x * con->max_y;
        memcpyw(con->memptr, con->bufptr, size);
    }
}

void vga_toggle_cursor(console_t *con)
{
    uint16_t tmp, pos;

    pos = (con->pos_y * con->max_x) + con->pos_x;
    tmp = con->bufptr[pos];
    tmp = ((tmp & 0xF000) >> 4) | ((tmp & 0x0F00) << 4)  | (tmp & 0x00FF);
    con->bufptr[pos] = tmp;

    if(con->active)
    {
        con->memptr[pos] = tmp;
    }
}

void vga_clear(console_t *con)
{
    uint16_t blank, memsz;

    blank = (con->attribute << 8) | 0x20;
    memsz = con->max_x * con->max_y;
    memsetw(con->bufptr, blank, memsz);

    if(con->active)
    {
        memcpyw(con->memptr, con->bufptr, memsz);
    }
}

void vga_set_color(console_t *con, int fg, int bg)
{
    int attr = con->attribute;

    if(fg >= 0 && fg <= 15)
    {
        attr = (attr & 0xF0) | fg;
    }

    if(bg >= 0 && bg <= 15)
    {
        attr = (attr & 0x0F) | (bg << 4);
    }

    con->attribute = attr;
}

void vga_putch(console_t *con, int ch)
{
    uint16_t pos;

    pos = (con->pos_y * con->max_x) + con->pos_x;
    con->bufptr[pos] = (con->attribute << 8) | ch;

    if(con->active)
    {
        con->memptr[pos] = con->bufptr[pos];
    }
}

void vga_init(console_t *con)
{
    con->max_x = 80;
    con->max_y = 25;
    con->attribute = 0x0F;

    con->memsz = con->max_x * con->max_y * sizeof(uint16_t);
    con->memptr = (uint16_t*)0xFFFFFFFF800B8000;
    con->bufptr = kzalloc(con->memsz);

    // disable hardware cursor
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, 0x20);
}
