#include <stdlib.h>
#include <stdio.h>
#include "ted.h"

line_t *line_grow(file_t *file, int num, int minsz)
{
    line_t *line;
    int cap;

    line = file->line[num];
    cap = 2 * line->cap;
    while(cap < minsz)
    {
        cap = 2 * cap;
    }

    line = realloc(line, sizeof(line_t) + cap);
    file->line[num] = line;
    line->cap = cap;

    return line;
}

void file_grow(file_t *file)
{
    line_t *line;
    int cap;

    cap = 2 * file->cap;
    if(!cap)
    {
        cap = 512;
    }
    file->line = realloc(file->line, sizeof(line_t*) * cap);

    for(int i = file->cap; i < cap; i++)
    {
        line = malloc(sizeof(line_t) + 128);
        file->line[i] = line;

        line->size = 0;
        line->cap = 128;
        line->data[0] = '\0';
    }

    file->cap = cap;
}

file_t *file_read(const char *filename)
{
    file_t *file;
    line_t *line;
    char *data;
    int lnum;
    int len;
    int pos;
    int ch;

    FILE *fp = fopen(filename, "r");
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        rewind(fp);

        data = malloc(len);
        fread(data, 1, len, fp);
        fclose(fp);
    }
    else
    {
        len = 0;
    }

    file = calloc(1, sizeof(file_t));
    file->size = 1;
    file->cap = 0;
    file->line = 0;
    file_grow(file);

    if(!len)
    {
        return file;
    }

    line = file->line[0];
    pos = 0;
    while(pos < len)
    {
        ch = data[pos];
        pos++;

        if(ch == '\n')
        {
            line = file->line[file->size];
            lnum = file->size++;
        }
        else
        {
            line->data[line->size] = ch;
            line->size++;
            line->data[line->size] = '\0';
        }

        if(file->size == file->cap)
        {
            file_grow(file);
            line = file->line[lnum];
        }

        if(line->size + 1 == line->cap)
        {
            line = line_grow(file, lnum, 0);
        }
    }

    free(data);
    return file;
}


int file_write(const char *filename, file_t *file)
{
    line_t *line;
    int last;

    FILE *fp = fopen(filename, "w");
    if(fp == NULL)
    {
        return -1;
    }

    last = file->size - 1;
    for(int i = 0; i < last; i++)
    {
        line = file->line[i];
        fputs(line->data, fp);
        fputc('\n', fp);
    }

    line = file->line[last];
    if(line->size)
    {
        fputs(line->data, fp);
        fputc('\n', fp);
    }

    fclose(fp);
    return 0;
}
