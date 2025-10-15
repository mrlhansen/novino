#include <string.h>
#include <stdio.h>
#include "ted.h"

void textarea_up(textarea_t *w, int num)
{
    line_t *line;
    file_t *file;

    file = w->file;

    if(w->wy.pos == 0)
    {
        return;
    }

    w->wy.pos = w->wy.pos - num;

    if(w->wy.pos < 0)
    {
        w->wy.pos = 0;
    }

    line = file->line[w->wy.pos];

    if(w->wx.pos > line->size)
    {
        w->wx.pos = line->size;
    }
}

void textarea_down(textarea_t *w, int num)
{
    line_t *line;
    file_t *file;

    file = w->file;

    if(w->wy.pos + 1 == file->size)
    {
        return;
    }

    w->wy.pos = w->wy.pos + num;

    if(w->wy.pos >= file->size)
    {
        w->wy.pos = file->size - 1;
    }

    line = file->line[w->wy.pos];

    if(w->wx.pos > line->size)
    {
        w->wx.pos = line->size;
    }
}

void textarea_right(textarea_t *w)
{
    file_t *file;
    line_t *line;

    file = w->file;
    line = file->line[w->wy.pos];

    if(w->wx.pos == line->size)
    {
        return;
    }

    w->wx.pos++;
}

void textarea_left(textarea_t *w)
{
    if(w->wx.pos == 0)
    {
        return;
    }
    w->wx.pos--;
}

void textarea_home(textarea_t *w)
{
    w->wx.pos = 0;
}

void textarea_end(textarea_t *w)
{
    file_t *file;
    line_t *line;

    file = w->file;
    line = file->line[w->wy.pos];
    w->wx.pos = line->size;
}

void textarea_enter(textarea_t *w)
{
    line_t *line;
    line_t *next;
    file_t *file;

    file = w->file;
    line = file->line[w->wy.pos];

    // move lines down
    next = file->line[file->size];
    for(int i = file->size; i > w->wy.pos; i--)
    {
        file->line[i] = file->line[i-1];
    }
    file->line[w->wy.pos+1] = next;
    file->size++;

    // new line
    next->size = line->size - w->wx.pos;
    if(next->size + 1 >= next->cap)
    {
        next = line_grow(file, w->wy.pos + 1, next->size);
    }
    strcpy(next->data, line->data + w->wx.pos);

    // shrink old line
    line->size = w->wx.pos;
    line->data[line->size] = '\0';

    // adjust positions
    w->wy.pos++;
    w->wx.pos = 0;

    // file capacity
    if(file->size == file->cap)
    {
        file_grow(file);
    }
}

void textarea_backspace(textarea_t *w)
{
    line_t *line;
    line_t *prev;
    file_t *file;
    int size;

    file = w->file;
    line = file->line[w->wy.pos];

    if(w->wx.pos > 0)
    {
        memmove(line->data + w->wx.pos - 1, line->data + w->wx.pos, line->size - w->wx.pos);
        line->size--;
        line->data[line->size] = '\0';
        w->wx.pos--;
    }
    else if(w->wy.pos > 0)
    {
        prev = file->line[w->wy.pos-1];
        w->wx.pos = prev->size;

        size = prev->size + line->size;
        if(size + 1 >= prev->cap)
        {
            prev = line_grow(file, w->wy.pos - 1, size);
        }
        strcpy(prev->data + prev->size, line->data);
        prev->size = size;

        for(int i = w->wy.pos; i < file->size; i++)
        {
            file->line[i] = file->line[i+1];
        }
        file->line[file->size] = line;
        file->size--;

        line->size = 0;
        line->data[0] = '\0';

        w->wy.pos--;
    }
}

void textarea_glyph(textarea_t *w, int key)
{
    line_t *line;
    file_t *file;

    file = w->file;
    line = file->line[w->wy.pos];

    memmove(line->data + w->wx.pos + 1, line->data + w->wx.pos, line->size - w->wx.pos);
    line->data[w->wx.pos] = key;
    line->size++;
    line->data[line->size] = '\0';
    w->wx.pos++;

    if(line->size + 1 == line->cap)
    {
        line_grow(file, w->wy.pos, 0);
    }
}
