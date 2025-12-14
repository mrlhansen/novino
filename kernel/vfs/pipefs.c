#include <kernel/mem/heap.h>
#include <kernel/vfs/pipes.h>
#include <kernel/vfs/fd.h>
#include <kernel/errno.h>

typedef struct {
    pipe_t *pipe;
    file_t *rd;
    file_t *wr;
} pipefs_t;

static int pipefs_read(file_t *file, size_t size, void *buf)
{
    pipefs_t *fs;

    fs = file->data;
    if(!fs->wr)
    {
        if(pipe_empty(fs->pipe))
        {
            return 0;
        }
    }

    return pipe_read(fs->pipe, size, buf);
}

static int pipefs_write(file_t *file, size_t size, void *buf)
{
    pipefs_t *fs;

    fs = file->data;
    if(!fs->rd)
    {
        return -EPIPE;
    }

    return pipe_write(fs->pipe, size, buf);
}

static int pipefs_close(file_t *file)
{
    pipefs_t *fs;

    fs = file->data;
    // we probably need a lock

    if(file == fs->rd)
    {
        fs->rd = 0;
    }

    if(file == fs->wr)
    {
        fs->wr = 0;
        if(fs->rd)
        {
            pipe_write(fs->pipe, 0, 0);
        }
    }

    if(!fs->rd && !fs->wr)
    {
        pipe_delete(fs->pipe);
        kfree(fs);
    }

    return 0;
}

static vfs_ops_t ops = {
    .close = pipefs_close,
    .read = pipefs_read,
    .write = pipefs_write,
};

static inode_t inode = {
    .flags = I_PIPE,
    .ops = &ops,
};

int vfs_mkpipe(int *fd)
{
    pipefs_t *fs;
    pipe_t *pipe;
    fd_t *rd;
    fd_t *wr;

    pipe = pipe_create(0, 0);
    if(!pipe)
    {
        return -ENOMEM;
    }

    rd = fd_create();
    if(!rd)
    {
        return -ENOMEM;
    }

    wr = fd_create();
    if(!wr)
    {
        return -ENOMEM;
    }

    fs = kzalloc(sizeof(*fs));
    if(!fs)
    {
        return -ENOMEM;
    }

    fs->pipe = pipe;
    fs->rd = rd->file;
    fs->wr = wr->file;

    rd->file->flags = O_READ;
    rd->file->inode = &inode;
    rd->file->data = fs;

    wr->file->flags = O_WRITE;
    wr->file->inode = &inode;
    wr->file->data = fs;

    fd[0] = rd->id;
    fd[1] = wr->id;

    return 0;
}
