#include <kernel/sched/process.h>
#include <kernel/mem/heap.h>
#include <kernel/vfs/fd.h>

fd_t *fd_create(file_t *file)
{
    process_t *process;
    fd_t *fd;

    fd = kzalloc(sizeof(fd_t));
    if(fd == 0)
    {
        return 0;
    }

    if(file == 0)
    {
        file = kzalloc(sizeof(file_t));
        if(file == 0)
        {
            kfree(fd);
            return 0;
        }
    }

    process = process_handle();
    fd->id = process->fd.next++;
    fd->file = file;
    file->links++;
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
    file_t *file;

    process = process_handle();
    list_remove(&process->fd.list, fd);

    file = fd->file;
    file->links--; // TODO: file context needs a lock
    kfree(fd);

    if(file->links == 0)
    {
        return file;
    }

    return 0;
}
