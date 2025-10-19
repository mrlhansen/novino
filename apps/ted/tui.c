#include <string.h>
#include <stdio.h>
#include "ted.h"

static void header(tui_t *tui)
{
    file_t *file;
    line_t *line;
    char row[16];
    char col[16];
    int len;

    len = strlen(tui->name);
    printf("\e[H\e[100;30m%s", tui->name);

    len = tui->cols - len - 32;
    for(int i = 0; i < len; i++)
    {
        putchar(' ');
    }

    file = tui->w.file;
    line = file->line[tui->w.wy.pos];

    sprintf(row, "%d/%d", 1+tui->w.wy.pos, file->size);
    sprintf(col, "%d/%d", 1+tui->w.wx.pos, line->size);
    printf("\e[47m L \e[100m %-12s", row);
    printf("\e[47m C \e[100m %-12s", col);
}

static void footer(tui_t *tui)
{
    printf("\e[%dH\e[30m", tui->rows);
    printf("\e[47m F1 \e[100m Save     ");
    printf("\e[47m F2 \e[100m Exit     ");
    for(int i = 28; i < tui->cols; i++)
    {
        putchar(' ');
    }
}

void tui_refresh(tui_t *tui)
{
    textarea_t *w;
    file_t *file;
    line_t *line;
    int lnum, row;
    int xoff, yoff;
    int s, e;

    w = &tui->w;
    file = w->file;
    xoff = w->wx.offset;
    yoff = w->wy.offset;

    // update cursor position
    w->x.pos = w->wx.pos - w->wx.offset;
    while(w->x.pos < 0)
    {
        w->x.pos++;
        w->wx.offset--;
    }
    while(w->x.pos >= w->x.size)
    {
        w->x.pos--;
        w->wx.offset++;
    }

    w->y.pos = w->wy.pos - w->wy.offset;
    while(w->y.pos < 0)
    {
        w->y.pos++;
        w->wy.offset--;
    }
    while(w->y.pos >= w->y.size)
    {
        w->y.pos--;
        w->wy.offset++;
    }

    // only redraw when needed
    xoff = xoff - w->wx.offset;
    yoff = yoff - w->wy.offset;
    s = 0;
    e = 0;

    if(xoff || yoff || w->r.all)
    {
        s = 0;
        e = w->y.size;
    }
    else if(w->r.line)
    {
        s = w->y.pos;
        e = s + 1;
    }

    // refresh screen
    header(tui);
    footer(tui);

    printf("\e[0m");
    w->r.all = 0;
    w->r.line = 0;

    for(int i = s; i < e; i++)
    {
        lnum = w->wy.offset + i;
        row = 1 + w->y.offset + i;

        if(lnum < file->size)
        {
            line = file->line[lnum];
            if(w->wx.offset < line->size)
            {
                printf("\e[%dH%*.*s", row, -w->x.size, w->x.size, line->data + w->wx.offset);
                continue;
            }
        }

        printf("\e[%dH%*s", row, w->x.size, "");
    }

    printf("\e[%d;%dH", 1 + w->y.offset + w->y.pos, 1 + w->x.offset + w->x.pos);
    fflush(stdout);
}
