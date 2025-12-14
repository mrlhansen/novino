#include <kernel/sched/process.h>
#include <kernel/mem/heap.h>
#include <kernel/vfs/fd.h>

fd_t *fd_create()
{
    process_t *process;
    file_t *file;
    fd_t *fd;

    fd = kzalloc(sizeof(fd_t));
    if(!fd)
    {
        return 0;
    }

    file = kzalloc(sizeof(file_t));
    if(!file)
    {
        kfree(fd);
        return 0;
    }

    process = process_handle();
    fd->id = process->fd.next++;
    fd->file = file;
    atomic_inc_fetch(&file->refs);
    list_insert(&process->fd.list, fd);

    return fd;
}

fd_t *fd_find(int id)
{
    process_t *process;
    list_t *list;
    fd_t *fd;

    process = process_handle();
    list = &process->fd.list;

    acquire_lock(&list->lock);
    fd = list->head;

    while(fd)
    {
        if(fd->id == id)
        {
            break;
        }
        fd = fd->link.next;
    }

    release_lock(&list->lock);

    return fd;
}

file_t *fd_delete(fd_t *fd)
{
    process_t *process;
    atomic_t refs;
    file_t *file;

    process = process_handle();
    list_remove(&process->fd.list, fd);

    file = fd->file;
    refs = atomic_dec_fetch(&file->refs);
    kfree(fd);

    if(refs == 0)
    {
        return file;
    }

    return 0;
}

fd_t *fd_clone(fd_t *fd)
{
    file_t *file;

    file = fd->file;
    fd = kzalloc(sizeof(fd_t));
    if(!fd)
    {
        return 0;
    }

    fd->file = file;
    atomic_inc_fetch(&file->refs);

    return fd;
}

void fd_adopt(fd_t *fd)
{
    process_t *process;
    process = process_handle();
    fd->id = process->fd.next++;
    list_insert(&process->fd.list, fd);
}
