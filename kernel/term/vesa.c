#include <kernel/term/console.h>
#include <kernel/term/fonts.h>
#include <kernel/term/psf.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/mmio.h>
#include <kernel/errno.h>
#include <string.h>

static psf_t font;
static int xspace = 0;
static int yspace = 0;

static inline void vesa_memcpy(uint64_t dst, uint64_t src, size_t len)
{
    size_t nl = len / 8;
    size_t ns = len % 8;

    for(int i = 0; i < nl; i++)
    {
        *(uint64_t*)dst = *(uint64_t*)src;
        dst += 8;
        src += 8;
    }

    for(int i = 0; i < ns; i++)
    {
        *(uint8_t*)dst = *(uint8_t*)src;
        dst++;
        src++;
    }
}

static inline void vesa_flush_line(console_t *con, int x0, int x1, int y)
{
    size_t pos, memsz;
    if(con->active)
    {
        pos = (y * con->bytes_per_line) + (x0 * con->bytes_per_pixel);
        memsz = (x1 - x0) * con->bytes_per_pixel;
        vesa_memcpy(con->lfbaddr + pos, con->bufaddr + pos, memsz);
    }
}

static inline void vesa_draw_line(console_t *con, int x0, int x1, int y, color_t *c)
{
    size_t pos = (y * con->bytes_per_line) + (x0 * con->bytes_per_pixel);
    uint8_t *buf = (void*)(con->bufaddr + pos);

    for(int x = x0; x < x1; x++)
    {
        for(int i = 0; i < c->nbytes; i++)
        {
            *buf++ = c->data[i];
        }
    }
}

static inline void vesa_putpixel(console_t *con, int x, int y, color_t *c)
{
    size_t pos = (y * con->bytes_per_line) + (x * con->bytes_per_pixel);
    uint8_t *buf = (void*)(con->bufaddr + pos);

    for(int i = 0; i < c->nbytes; i++)
    {
        buf[i] = c->data[i];
    }
}

static void vesa_encode_color(int bits, uint32_t color, color_t *c)
{
    switch(bits)
    {
        case 15: // (5:5:5)
            c->nbytes  = 2;
            c->blue    = (color >> 3)  & 0x1F;
            c->green   = (color >> 11) & 0x1F;
            c->red     = (color >> 19) & 0x1F;
            c->data[0] = (c->green << 5) | c->blue;
            c->data[1] = (c->red << 2) | (c->green >> 3);
            break;
        case 16: // (5:6:5)
            c->nbytes  = 2;
            c->blue    = (color >> 3)  & 0x1F;
            c->green   = (color >> 10) & 0x3F;
            c->red     = (color >> 19) & 0x1F;
            c->data[0] = (c->green << 5) | c->blue;
            c->data[1] = (c->red << 3) | (c->green >> 3);
            break;
        case 24: // (8:8:8)
        case 32:
            c->nbytes  = (bits / 8);
            c->blue    = (color >> 0)  & 0xFF;
            c->green   = (color >> 8)  & 0xFF;
            c->red     = (color >> 16) & 0xFF;
            c->data[0] = c->blue;
            c->data[1] = c->green;
            c->data[2] = c->red;
            c->data[4] = 0;
            break;
    }
}

void vesa_set_color(console_t *con, int fg, int bg)
{
    const uint32_t color[16] = {
        0x000000,
        0x0000C0,
        0x00C000,
        0x00C0C0,
        0xC00000,
        0xC000C0,
        0xC08000,
        0xC0C0C0,
        0x808080,
        0x0000FF,
        0x00FF00,
        0x00FFFF,
        0xFF0000,
        0xFF00FF,
        0xFFFF00,
        0xFFFFFF
    };

    if(fg >= 0 && fg <= 15)
    {
        vesa_encode_color(con->depth, color[fg], &con->fg);
    }

    if(bg >= 0 && bg <= 15)
    {
        vesa_encode_color(con->depth, color[bg], &con->bg);
    }
}

void vesa_scroll(console_t *con)
{
    int row, size, y0;

    y0   = (font.height + yspace) * (con->max_y - 1);
    row  = (font.height + yspace) * con->bytes_per_line;
    size = row * (con->max_y - 1);

    // Redraw
    vesa_memcpy(con->bufaddr, con->bufaddr + row, size);
    for(int y = y0; y < con->height; y++)
    {
        vesa_draw_line(con, 0, con->width, y, &con->bg);
    }

    // Flush
    if(con->active)
    {
        vesa_memcpy(con->lfbaddr, con->bufaddr, con->memsz);
    }

    // Glyphs
    size = con->max_x * (con->max_y - 1);
    for(int i = 0; i < size; i++)
    {
        con->glyphs[i].ch = con->glyphs[i + con->max_x].ch;
        con->glyphs[i].bg = con->glyphs[i + con->max_x].fg;
        con->glyphs[i].bg = con->glyphs[i + con->max_x].bg;
    }
    for(int i = 0; i < con->max_x; i++)
    {
        con->glyphs[i + size].ch = ' ';
        con->glyphs[i + size].fg = con->fg;
        con->glyphs[i + size].bg = con->bg;
    }
}

void vesa_refresh(console_t *con)
{
    if(con->active)
    {
        vesa_memcpy(con->lfbaddr, con->bufaddr, con->memsz);
    }
}

void vesa_clear(console_t *con)
{
    for(int y = 0; y < con->height; y++)
    {
        vesa_draw_line(con, 0, con->width, y, &con->bg);
        vesa_flush_line(con, 0, con->width, y);
    }

    for(int i = 0; i < (con->max_x * con->max_y); i++)
    {
        con->glyphs[i].ch = ' ';
        con->glyphs[i].fg = con->fg;
        con->glyphs[i].bg = con->bg;
    }
}

void vesa_toggle_cursor(console_t *con)
{
    glyph_t *glyph;
    color_t fg, bg;

    glyph = con->glyphs + con->pos_x + (con->pos_y * con->max_x);
    fg = con->fg;
    bg = con->bg;

    con->fg = glyph->bg;
    con->bg = glyph->fg;
    vesa_putch(con, glyph->ch);
    con->fg = fg;
    con->bg = bg;
}

void vesa_putch(console_t *con, int ch)
{
    uint32_t row, mask;
    int x, y, x0, y0;
    uint8_t *data;

    if(ch > 127)
    {
        ch = psf_unicode(&font, ch);
    }

    data = font.glyphs + (ch * font.bytes_per_glyph);
    x0 = con->pos_x * (font.width + xspace);
    y0 = con->pos_y * (font.height + yspace);
    x = x0;
    y = y0;

    for(int i = 0; i < font.height; i++)
    {
        row = 0;
        for(int j = 0; j < font.bytes_per_row; j++)
        {
            row = (row << 8) | *data++;
        }
        row >>= font.padding;
        mask = 1 << (font.width - 1);

        while(mask)
        {
            if(row & mask)
            {
                vesa_putpixel(con, x, y, &con->fg);
            }
            else
            {
                vesa_putpixel(con, x, y, &con->bg);
            }
            mask >>= 1;
            x++;
        }

        for(int j = 0; j < xspace; j++)
        {
            vesa_putpixel(con, x, y, &con->bg);
            x++;
        }

        vesa_flush_line(con, x0, x, y);
        x = x0;
        y++;
    }

    for(int j = 0; j < yspace; j++)
    {
        vesa_draw_line(con, x, x + font.width + xspace, y, &con->bg);
        vesa_flush_line(con, x, x + font.width + xspace, y);
        x = x0;
        y++;
    }

    row = con->pos_x + (con->pos_y * con->max_x);
    con->glyphs[row].ch = ch;
    con->glyphs[row].fg = con->fg;
    con->glyphs[row].bg = con->bg;
}

int vesa_reinit(console_t *con)
{
    uint64_t virt;
    int status;

    status = mmio_map_wc_region(con->lfbaddr, con->memsz, &virt);
    if(status)
    {
        return status;
    }

    con->lfbaddr = virt;
    return 0;
}

void vesa_init(console_t *con, bootstruct_t *bs)
{
    con->width           = bs->lfb_width;
    con->height          = bs->lfb_height;
    con->depth           = bs->lfb_bpp;
    con->bytes_per_line  = bs->lfb_pitch;
    con->bytes_per_pixel = (bs->lfb_bpp + 7) / 8;
    con->memsz           = con->bytes_per_line * con->height;
    con->lfbphys         = bs->lfb_address;
    con->lfbaddr         = bs->lfb_address;
    con->bufaddr         = (uint64_t)kzalloc(con->memsz);

    xspace = 1;
    yspace = 0;
    psf_parse(&font, zap_light_8x16);
    vesa_set_color(con, VGA_BRIGHT_WHITE, VGA_BLACK);

    con->max_x  = con->width / (font.width + xspace);
    con->max_y  = con->height / (font.height + yspace);
    con->glyphs = kcalloc(con->max_x*con->max_y, sizeof(glyph_t));
}
