#include <_syscall.h>
#include <_stdlib.h>
#include <_stdio.h>

static FILE *list = 0;

FILE *__libc_fd_alloc(int fd)
{
    FILE *fp = malloc(sizeof(FILE) + BUFSIZ + UNGETSIZ);
    if(fp == NULL)
    {
        return NULL;
    }

    fp->fd = fd;
    fp->flags = 0;
    fp->mode = 0;
    fp->b.ptr = (char*)(fp+1);
    fp->b.pos = fp->b.ptr;
    fp->b.end = fp->b.ptr;
    fp->b.size = BUFSIZ;
    fp->b.mode = _IOFBF;
    fp->u.ptr = fp->b.ptr + BUFSIZ;
    fp->u.pos = fp->u.ptr;

    if(fd < 2)
    {
        fp->b.mode = _IOLBF;
    }

    fp->next = list;
    fp->prev = 0;
    if(list)
    {
        list->prev = fp;
    }
    list = fp;

    return fp;
}

void __libc_fd_free(FILE *fp)
{
    if(list == fp)
    {
        list = fp->next;
    }
    if(fp->next)
    {
        fp->next->prev = fp->prev;
    }
    if(fp->prev)
    {
        fp->prev->next = fp->next;
    }
    free(fp);
}

void __libc_fd_exit()
{
    while(list)
    {
        __libc_fp_flush(list);
        sys_close(list->fd);
        list = list->next;
    }
}

long __libc_fp_write(FILE *fp, size_t size, void *ptr)
{
    long status;

    status = sys_write(fp->fd, size, ptr);
    if(status < size)
    {
        errno = -status;
        fp->flags |= F_ERR;
        return status;
    }

    return status;
}

long __libc_fp_read(FILE *fp, size_t size, void *ptr)
{
    long status;

    status = sys_read(fp->fd, size, ptr);
    if(status < 0)
    {
        errno = -status;
        fp->flags |= F_ERR;
        return status;
    }
    if(status == 0)
    {
        fp->flags |= F_EOF;
    }

    return status;
}

long __libc_fp_flush(FILE *fp)
{
    size_t size;
    long status;

    if(fp->b.mode == _IONBF)
    {
        return 0;
    }

    if(fp->mode != F_WRITE)
    {
        return 0;
    }

    size = (size_t)(fp->b.pos - fp->b.ptr);
    status = __libc_fp_write(fp, size, fp->b.ptr);
    if(status < 0)
    {
        return status;
    }

    fp->b.pos = fp->b.ptr;
    fp->b.end = fp->b.ptr + fp->b.size;

    return status;
}
