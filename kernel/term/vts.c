#include <kernel/vfs/devfs.h>
#include <kernel/term/term.h>
#include <kernel/term/vts.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>
#include <stdio.h>

// We use /devices/vtsX for virtual terminals
// We use /devices/ptsX for pseudo terminals

#define MAXVTS 8
static vts_t vts[MAXVTS];

static int vts_open(file_t *file)
{
    vts_t *vts = file->data;
    if(vts->file)
    {
        return -EBUSY;
    }
    vts->file = file;
    return 0;
}

static int vts_close(file_t *file)
{
    vts_t *vts = file->data;
    if(vts->file == file)
    {
        vts->file = 0;
    }
    return 0;
}

static int vts_read(file_t *file, size_t size, void *buf)
{
    vts_t *vts = file->data;
    int status;

    vts->rdwait = 1;
    status = pipe_read(vts->pipe, size, buf);
    vts->rdwait = 0;

    return status;
}

static int vts_write(file_t *file, size_t size, void *buf)
{
    term_stdout(file->data, buf, size);
    return size;
}

static int vts_ioctl(file_t *file, size_t cmd, size_t val)
{
    winsz_t *winsz = (void*)val;
    vts_t *vts = file->data;

    if(cmd == TIOGETFLAGS)
    {
        *(uint32_t*)val = vts->flags;
    }
    else if(cmd == TIOSETFLAGS)
    {
        vts->flags = val;
        console_set_flags(vts->console, val);
    }
    else if(cmd == TIOGETWINSZ)
    {
        winsz->cols = vts->console->max_x;
        winsz->rows = vts->console->max_y;
        winsz->xpixel = vts->console->width;
        winsz->ypixel = vts->console->height;
    }
    else
    {
        return -ENOIOCTL;
    }

    return 0;
}

void vts_flush_input(vts_t *vts)
{
    pipe_write(vts->pipe, vts->ipos, vts->ibuf);
    memset(vts->ibuf, 0, sizeof(vts->ibuf));
    vts->ipos = 0;
}

vts_t *vts_select(int num)
{
    if(num >= MAXVTS)
    {
        return 0;
    }
    return vts + num;
}

void vts_init()
{
    char name[16];

    static devfs_ops_t ops = {
        .open = vts_open,
        .close = vts_close,
        .read = vts_read,
        .write = vts_write,
        .seek = 0,
        .ioctl = vts_ioctl,
    };

    for(int i = 0; i < MAXVTS; i++)
    {
        vts[i].id = i;
        vts[i].file = 0;
        vts[i].pipe = pipe_create(0, O_NONBLOCK);
        vts[i].rdwait = 0;
        vts[i].ipos = 0;
        vts[i].flags = (ECHO | CURSOR);

        if(i == 0)
        {
            vts[i].console = console_default();
            vts[i].flags = 0;
        }
        else
        {
            vts[i].console = console_clone();
            console_set_flags(vts[i].console, vts[i].flags);
        }

        sprintf(name, "vts%d", i);
        devfs_stream_register(0, name, &ops, &vts[i], 0);
    }
}
