#include <kernel/term/console.h>
#include <kernel/term/term.h>
#include <kernel/mem/heap.h>

static int vesamode = 0;
static console_t console;

static inline void console_toggle_cursor(console_t *c)
{
    if(c->flags & CURSOR)
    {
        if(vesamode)
        {
            vesa_toggle_cursor(c);
        }
        else
        {
            vga_toggle_cursor(c);
        }
    }
}

static inline void console_scroll(console_t *c)
{
    if(c->pos_y < c->max_y)
    {
        return;
    }

    if((c->flags & NOSCRL) == 0)
    {
        if(vesamode)
        {
            vesa_scroll(c);
        }
        else
        {
            vga_scroll(c);
        }
    }

    c->pos_y--;
    c->pos_x = 0;
}

static void console_putchar(console_t *c, int ch)
{
    if(ch == '\n')
    {
        c->pos_y++;
        c->pos_x = 0;
    }
    else if(ch == '\b')
    {
        if(c->pos_x > 0)
        {
            c->pos_x--;
        }
        else if(c->pos_y > 0)
        {
            c->pos_y--;
            c->pos_x = c->max_x - 1;
        }
    }
    else if(ch == '\r')
    {
        c->pos_x = 0;
    }
    else if(ch == '\t')
    {
        c->pos_x = (c->pos_x + 8) & 0xFFF8;
        if(c->pos_x >= c->max_x)
        {
            c->pos_x -= c->max_x;
            c->pos_y++;
        }
    }
    else if(ch >= 0x10)
    {
        if(vesamode)
        {
            vesa_putch(c, ch);
        }
        else
        {
            vga_putch(c, ch);
        }

        if((c->pos_x + 1) == c->max_x)
        {
            c->pos_x = 0;
            c->pos_y++;
        }
        else
        {
            c->pos_x++;
        }
    }

    console_scroll(c);
}

void console_set_flags(console_t *c, int flags)
{
    acquire_lock(&c->lock);

    if((flags ^ c->flags) & CURSOR)
    {
        if(flags & CURSOR)
        {
            c->flags = flags;
            console_toggle_cursor(c);
        }
        else
        {
            console_toggle_cursor(c);
            c->flags = flags;
        }
    }
    else
    {
        c->flags = flags;
    }

    release_lock(&c->lock);
}

void console_move_cursor(console_t *c, int x, int y, bool rela)
{
    acquire_lock(&c->lock);
    console_toggle_cursor(c);

    if(rela)
    {
        x = x + c->pos_x;
        y = y + c->pos_y;
    }

    if(c->flags & WRAP)
    {
        while(x < 0)
        {
            x = x + c->max_x;
            y = y - 1;
        }

        while(x >= c->max_x)
        {
            x = x - c->max_x;
            y = y + 1;
        }
    }

    if(x < 0)
    {
        c->pos_x = 0;
    }
    else if(x >= c->max_x)
    {
        c->pos_x = c->max_x - 1;
    }
    else
    {
        c->pos_x = x;
    }

    if(y < 0)
    {
        c->pos_y = 0;
    }
    else if(y >= c->max_y)
    {
        c->pos_y = c->max_y - 1;
    }
    else
    {
        c->pos_y = y;
    }

    console_toggle_cursor(c);
    release_lock(&c->lock);
}

void console_set_color(console_t *c, int fg, int bg)
{
    acquire_lock(&c->lock);
    if(vesamode)
    {
        vesa_set_color(c, fg, bg);
    }
    else
    {
        vga_set_color(c, fg, bg);
    }
    release_lock(&c->lock);
}

void console_clear(console_t *c)
{
    acquire_lock(&c->lock);

    c->pos_x = 0;
    c->pos_y = 0;

    if(vesamode)
    {
        vesa_clear(c);
    }
    else
    {
        vga_clear(c);
    }

    console_toggle_cursor(c);
    release_lock(&c->lock);
}

void console_refresh(console_t *c)
{
    acquire_lock(&c->lock);
    if(vesamode)
    {
        vesa_refresh(c);
    }
    else
    {
        vga_refresh(c);

    }
    release_lock(&c->lock);
}

void console_putch(console_t *c, int ch)
{
    acquire_lock(&c->lock);
    console_toggle_cursor(c);
    console_putchar(c, ch);
    console_toggle_cursor(c);
    release_lock(&c->lock);
}

void console_puts(console_t *c, char *str)
{
    acquire_lock(&c->lock);
    console_toggle_cursor(c);
    while(*str)
    {
        console_putchar(c, *str);
        str++;
    }
    console_toggle_cursor(c);
    release_lock(&c->lock);
}

console_t *console_clone()
{
    console_t *c;

    c = kzalloc(sizeof(console_t) + console.memsz);
    if(c == 0)
    {
        return 0;
    }

    *c = console;
    c->active = 0;

    if(vesamode)
    {
        c->bufaddr = (uint64_t)(c+1);
        c->glyphs  = kcalloc(c->max_x*c->max_y, sizeof(glyph_t));
    }
    else
    {
        c->bufptr = (uint16_t*)(c+1);
    }

    console_clear(c);

    return c;
}

console_t *console_default()
{
    return &console;
}

void console_init(bootstruct_t *bs)
{
    // Reinitialize after paging has been enabled
    if(bs == 0)
    {
        if(vesamode)
        {
            vesa_reinit(&console);
        }
        return;
    }

    // Normal initialization
    console.active = 1;
    console.flags = 0;

    if(bs->lfb_mode == 0)
    {
        vesamode = 0;
        vga_init(&console);
    }
    else
    {
        vesamode = 1;
        vesa_init(&console, bs);
    }

    console_clear(&console);
}
