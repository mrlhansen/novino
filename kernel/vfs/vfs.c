#include <kernel/vfs/devfs.h>
#include <kernel/vfs/vfs.h>
#include <kernel/vfs/fd.h>
#include <kernel/mem/heap.h>
#include <kernel/sched/process.h>
#include <kernel/cleanup.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

DEFINE_AUTOFREE_TYPE(vfs_path_t)

static LIST_INIT(fsl, vfs_fs_t, link);
static LIST_INIT(mpl, vfs_mp_t, link);
static dentry_t *root = 0;

static int vfs_validate_name(const char *name, int maxlen)
{
    if(strlen(name) >= maxlen)
    {
        return -ENAMETOOLONG;
    }

    if(strcmp(name, ".") == 0)
    {
        return -EINVAL;
    }

    if(strcmp(name, "..") == 0)
    {
        return -EINVAL;
    }

    if(strchr(name, '/') != NULL)
    {
        return -EINVAL;
    }

    return 0;
}

static vfs_fs_t *vfs_find_fs(const char *name)
{
    vfs_fs_t *curr = fsl.head;

    while(curr)
    {
        if(strcmp(curr->name, name) == 0)
        {
            return curr;
        }
        curr = curr->link.next;
    }

    return 0;
}

static vfs_mp_t *vfs_find_mp(const char *name)
{
    vfs_mp_t *curr = mpl.head;

    while(curr)
    {
        if(strcmp(curr->name, name) == 0)
        {
            return curr;
        }
        curr = curr->link.next;
    }

    return 0;
}

//  This function will clean a path by
//  - adding leading slash if missing
//  - removing trailing slashes
//  - removing repeated slashes
//  - removing . entries
//  - resolving .. entries that do not cross the root
//
//  Some examples
//  - a/simple/path -> /a/simple/path
//  - /my/dir/ -> /my/dir
//  - /my/deep/../path -> /my/path
//  - /root/../../var/log/.//system -> /../var/log/system
//
static int vfs_clean_path(char *dest, const char *path)
{
    int len, stop, depth;
    int dot, dotdot, prev;
    const char *e, *s;

    dotdot = 0;
    dot = 0;

    depth = 0;
    stop = 0;
    s = 0;
    e = 0;

    if(*path != '/')
    {
        s = path;
    }

    while(stop == 0)
    {
        if(*path == '\0')
        {
            e = path;
            stop = 1;
        }
        else if(*path == '/')
        {
            if(s == 0)
            {
                s = path + 1;
            }
            else
            {
                e = path;
            }
        }

        if(s && e)
        {
            len = (int)(e-s);

            prev = dotdot;
            dot = ((len == 1) && (s[0] == '.'));
            dotdot = ((len == 2) && (s[0] == '.') && (s[1] == '.'));

            if(dotdot && depth && !prev)
            {
                while(*dest != '/')
                {
                    *dest-- = '\0';
                }

                *dest = '\0';
                depth--;

                // need to check if previous entry was ..
                if(depth)
                {
                    s = dest - 1;
                    while(*s != '/')
                    {
                        s--;
                    }
                    s++;
                    len = (int)(dest-s);
                    dotdot = ((len == 2) && (s[0] == '.') && (s[1] == '.'));
                }
                else
                {
                    dotdot = 0;
                }
            }
            else if(!dot && len)
            {
                *dest++ = '/';
                while(s < e)
                {
                    *dest++ = *s++;
                }
                *dest = '\0';
                depth++;
            }

            s = e + 1;
            e = 0;
        }

        path++;
    }

    if(depth == 0)
    {
        *dest++ = '/';
        *dest = '\0';
    }

    return depth;
}

static vfs_path_t *vfs_new_path(const char *str)
{
    vfs_path_t *path;
    process_t *pr;
    int memsz;

    // the cleaned path can at most be +1 longer than input path
    // so +2 below because of '\0' and because '/' might be added
    memsz = 2 + strlen(str) + sizeof(vfs_path_t);
    path = kzalloc(memsz);
    if(path == 0)
    {
        return 0;
    }

    // absolute or relative path
    if(*str == '/')
    {
        path->root = root;
    }
    else
    {
        pr = process_handle();
        if(pr->cwd)
        {
            path->root = pr->cwd;
        }
        else
        {
            path->root = root;
        }
    }

    // clean the path
    path->prev = 0;
    path->curr = 0;
    path->pos = 0;
    path->depth = vfs_clean_path(path->path, str);

    return path;
}

static char *vfs_read_path(vfs_path_t *path)
{
    if(path->curr == 0)
    {
        path->prev = 0;
        path->curr = strtok_r(path->path, "/", &path->pos);
    }
    else
    {
        path->prev = path->curr;
        path->curr = strtok_r(NULL, "/", &path->pos);
    }
    return path->curr;
}

static int vfs_walk_path(const char *pathname, dentry_t **dp)
{
    autofree(vfs_path_t) *path;
    dentry_t *parent, *child;
    inode_t inode, *ip;
    vfs_mp_t *mp;
    vfs_fs_t *fs;
    int status;

    path = vfs_new_path(pathname);
    if(path->depth == 0)
    {
        *dp = path->root;
        return 0;
    }

    parent = path->root;
    *dp = parent;

    while(vfs_read_path(path))
    {
        if((parent->inode->flags & I_DIR) == 0)
        {
            return -ENOTDIR;
        }

        if(strcmp(path->curr, "..") == 0)
        {
            parent = parent->parent;
            *dp = parent;
            continue;
        }

        if(parent == root)
        {
            mp = vfs_find_mp(path->curr);
            if(mp == 0)
            {
                return -ENOENT;
            }

            parent = &mp->dentry;
            *dp = parent;
            continue;
        }

        child = dcache_lookup(parent, path->curr);
        if(child)
        {
            if(!child->inode)
            {
                return -ENOENT;
            }
        }
        else
        {
            ip = &inode;
            fs = parent->inode->fs;
            memset(ip, 0, sizeof(inode));

            status = fs->ops->lookup(parent->inode, path->curr, ip);
            if(status < 0)
            {
                ip = 0;
            }

            child = dcache_append(parent, path->curr, ip);
            if(child == 0)
            {
                return -ENOMEM;
            }
            if(status < 0)
            {
                return status;
            }
        }

        *dp = child;
        parent = child;
    }

    return 0;
}

int vfs_put_dirent(void *data, const char *name, inode_t *inode)
{
    dentry_t *parent, *child;
    dirent_t *dirent;
    vfs_rd_t *rd;
    int len;

    rd = data;
    parent = rd->parent;

    // omit extraneous . and .. (not sure about this yet, we might need to force the fs code to not put them)
    if(rd->file->seek >= 2)
    {
        if(strcmp(name, ".") == 0)
        {
            return 0;
        }
        if(strcmp(name, "..") == 0)
        {
            return 0;
        }
    }

    // append to dirent
    dirent = (dirent_t*)(rd->dirent + rd->pos);
    len = 1 + sizeof(dirent_t) + strlen(name);

    if(rd->pos + len < rd->size)
    {
        rd->file->seek++;
        rd->pos += len;
        rd->status = rd->pos;

        dirent->length = len;
        dirent->flags = inode->flags;
        strcpy(dirent->name, name);
    }
    else
    {
        if(!rd->pos)
        {
            rd->status = -EINVAL;
        }
        return -ENOSPC;
    }

    // append to dcache
    if(!parent)
    {
        return 0;
    }

    child = dcache_lookup(parent, name);
    if(child == 0)
    {
        dcache_append(parent, name, inode);
    }

    return 0;
}

int vfs_readdir(int id, size_t size, dirent_t *dirent)
{
    int seek, status;
    vfs_mp_t *mp;
    vfs_fs_t *fs;
    dentry_t *dp;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    if((fd->file->flags & O_DIR) == 0)
    {
        return -ENOTDIR;
    }

    seek = fd->file->seek;
    dp = fd->file->dentry;
    fs = dp->inode->fs;
    mp = mpl.head;

    vfs_rd_t rd = {
        .file = fd->file,
        .parent = 0,
        .status = 0,
        .dirent = dirent,
        .size = size,
        .pos = 0,
    };

    // special case for . entry
    if(fd->file->seek == 0)
    {
        status = vfs_put_dirent(&rd, ".", dp->inode);
        if(status < 0)
        {
            return rd.status;
        }
    }

    // special case for .. entry
    if(fd->file->seek == 1)
    {
        status = vfs_put_dirent(&rd, "..", dp->parent->inode);
        if(status < 0)
        {
            return rd.status;
        }
    }

    // adjust seek
    seek = fd->file->seek - 2;

    // special case for root
    if(dp == root)
    {
        while(seek && mp)
        {
            seek--;
            mp = mp->link.next;
        }

        while(mp)
        {
            status = vfs_put_dirent(&rd, mp->name, &mp->inode);
            if(status < 0)
            {
                return rd.status;
            }
            mp = mp->link.next;
        }

        return rd.status;
    }

    // cache directory content
    if(dp->cached == 0)
    {
        if(1) // enable caching
        {
            rd.parent = dp;
        }

        status = fs->ops->readdir(fd->file, seek, &rd);
        if(status < 0)
        {
            return status;
        }

        if(rd.status == 0)
        {
            if(fd->file->seek == dp->positive + 2)
            {
                dp->cached = 1;
            }
        }

        return rd.status;
    }

    // seek to the correct position
    dp = dp->child;
    while(seek && dp)
    {
        if(dp->inode)
        {
            seek--;
        }
        dp = dp->next;
    }

    // append entries
    while(dp)
    {
        if(!dp->inode)
        {
            dp = dp->next;
            continue;
        }

        status = vfs_put_dirent(&rd, dp->name, dp->inode);
        if(status < 0)
        {
            return rd.status;
        }
        dp = dp->next;
    }

    return rd.status;
}

int vfs_fstat(int id, stat_t *stat)
{
    inode_t *ip;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    ip = fd->file->inode;
    stat->flags = ip->flags;
    stat->mode = ip->mode;
    stat->uid = ip->uid;
    stat->gid = ip->gid;
    stat->size = ip->size;
    stat->blksz = ip->blksz;
    stat->blocks = ip->blocks;
    stat->atime = ip->atime;
    stat->ctime = ip->ctime;
    stat->mtime = ip->mtime;

    return 0;
}

int vfs_stat(const char *pathname, stat_t *stat)
{
    dentry_t *dp;
    inode_t *ip;
    int status;

    status = vfs_walk_path(pathname, &dp);
    if(status < 0)
    {
        return status;
    }

    ip = dp->inode;
    stat->flags = ip->flags;
    stat->mode = ip->mode;
    stat->uid = ip->uid;
    stat->gid = ip->gid;
    stat->size = ip->size;
    stat->blksz = ip->blksz;
    stat->blocks = ip->blocks;
    stat->atime = ip->atime;
    stat->ctime = ip->ctime;
    stat->mtime = ip->mtime;

    return 0;
}

int vfs_read(int id, size_t size, void *buf)
{
    vfs_fs_t *fs;
    inode_t *ip;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    if(fd->file->flags & O_DIR)
    {
        return -EISDIR;
    }

    if((fd->file->flags & O_READ) == 0)
    {
        return -EBADF;
    }

    ip = fd->file->inode;
    fs = ip->fs;

    return fs->ops->read(fd->file, size, buf);
}

int vfs_write(int id, size_t size, void *buf)
{
    vfs_fs_t *fs;
    inode_t *ip;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    if(fd->file->flags & O_DIR)
    {
        return -EISDIR;
    }

    if((fd->file->flags & O_WRITE) == 0)
    {
        return -EBADF;
    }

    ip = fd->file->inode;
    fs = ip->fs;

    return fs->ops->write(fd->file, size, buf);
}

int vfs_seek(int id, long offset, int origin)
{
    vfs_fs_t *fs;
    inode_t *ip;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    if(fd->file->flags & O_DIR)
    {
        return -EISDIR; // We might want to support this!
    }

    ip = fd->file->inode;
    fs = ip->fs;

    if(fs->ops->seek == 0)
    {
        return -ENOTSUP;
    }

    return fs->ops->seek(fd->file, offset, origin);
}

int vfs_ioctl(int id, size_t cmd, size_t val)
{
    vfs_fs_t *fs;
    inode_t *ip;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    if(fd->file->flags & O_DIR)
    {
        return -EISDIR;
    }

    ip = fd->file->inode;
    fs = ip->fs;

    if(fs->ops->ioctl == 0)
    {
        return -ENOTSUP;
    }

    return fs->ops->ioctl(fd->file, cmd, val);
}

int vfs_chdir(const char *pathname)
{
    process_t *pr;
    dentry_t *dp;
    int status;

    status = vfs_walk_path(pathname, &dp);
    if(status < 0)
    {
        return status;
    }

    if((dp->inode->flags & I_DIR) == 0)
    {
        return -ENOTDIR;
    }

    pr = process_handle();
    pr->cwd = dp;

    return 0;
}

int vfs_getcwd(char *pathname, int size)
{
    process_t *pr;
    dentry_t *dp;
    char *s;
    int len;

    pr = process_handle();
    dp = pr->cwd;
    if(dp == 0)
    {
        dp = root;
    }
    s = pathname;

    if(size == 0)
    {
        return -EINVAL;
    }
    else if(size < 2)
    {
        return -ERANGE;
    }

    size = size - 2; // to always have room for '/' and '\0'

    if(dp == root)
    {
        *s++ = '/';
        *s = '\0';
    }
    else while(dp != root)
    {
        len = strlen(dp->name);
        if(len > size)
        {
            return -ERANGE;
        }

        for(int i = len-1; i >= 0; i--)
        {
            *s++ = dp->name[i];
        }

        *s++ = '/';
        *s = '\0';
        dp = dp->parent;
    }

    strrev(pathname);
    return 0;
}

int vfs_open(const char *pathname, int flags)
{
    vfs_fs_t *fs;
    dentry_t *dp;
    inode_t *ip;
    int status;
    fd_t *fd;

    status = vfs_walk_path(pathname, &dp);
    if(status < 0)
    {
        return status;
    }

    ip = dp->inode;
    fs = ip->fs;

    if(flags & O_DIR)
    {
        if((ip->flags & I_DIR) == 0)
        {
            return -ENOTDIR;
        }
    }
    else
    {
        if(ip->flags & I_DIR)
        {
            return -EISDIR;
        }
    }

    if(flags & O_READ)
    {
        if(fs->ops->read == 0)
        {
            return -ENOTSUP;
        }
    }

    if(flags & O_WRITE)
    {
        if(fs->ops->write == 0)
        {
            return -EROFS;
        }
    }

    fd = fd_create(0);
    fd->file->flags = flags;
    fd->file->dentry = dp;
    fd->file->inode = ip;

    if(fs->ops->open)
    {
        status = fs->ops->open(fd->file);
        if(status < 0)
        {
            fd_delete(fd);
            return status;
        }
    }

    return fd->id;
}

int vfs_close(int id)
{
    vfs_fs_t *fs;
    inode_t *ip;
    file_t *file;
    int status;
    fd_t *fd;

    fd = fd_find(id);
    if(fd == 0)
    {
        return -EBADF;
    }

    // fd_delete() returns the file context when there are no more references
    // and then we proceed to close the file

    file = fd_delete(fd);
    if(file == 0)
    {
        return 0;
    }

    ip = file->inode;
    fs = ip->fs;

    if(fs->ops->close)
    {
        status = fs->ops->close(file);
    }
    else
    {
        status = 0;
    }

    kfree(file);
    return status;
}

int vfs_mount(const char *device, const char *fsn, const char *mpn)
{
    vfs_mp_t *mp;
    vfs_fs_t *fs;
    devfs_t *dev;
    dentry_t *dp;
    inode_t inode;
    void *data;
    int status;

    if((fsn == 0) || (mpn == 0))
    {
        return -EINVAL;
    }

    status = vfs_validate_name(mpn, sizeof(mp->name));
    if(status)
    {
        return status;
    }

    fs = vfs_find_fs(fsn);
    if(fs == 0)
    {
        return -ENOFS;
    }

    if(vfs_find_mp(mpn))
    {
        return -EEXIST;
    }

    if(device)
    {
        status = vfs_walk_path(device, &dp);
        if(status < 0)
        {
            return status;
        }

        if((dp->inode->flags & I_BLOCK) == 0)
        {
            return -ENODEV;
        }

        dev = dp->inode->obj;
    }
    else
    {
        dev = 0;
    }

    data = fs->ops->mount(dev, &inode);
    if(data == 0)
    {
        return -EFAIL;
    }

    mp = kzalloc(sizeof(vfs_mp_t));
    if(mp == 0)
    {
        return -ENOMEM;
    }

    strcpy(mp->name, mpn);
    mp->fs = fs;
    mp->dev = dev;

    mp->inode = inode;
    mp->inode.fs = fs;
    mp->inode.mp = mp;
    mp->inode.data = data;

    strcpy(mp->dentry.name, mpn);
    mp->dentry.inode = &mp->inode;
    mp->dentry.parent = root;

    list_insert(&mpl, mp);

    kp_info("vfs", "mounted %s on /%s", fsn, mpn);
    return 0;
}

int vfs_register(const char *name, vfs_ops_t *ops)
{
    vfs_fs_t *fs;
    int status;

    status = vfs_validate_name(name, sizeof(fs->name));
    if(status)
    {
        return status;
    }

    if(vfs_find_fs(name))
    {
        return -ENOFS;
    }

    if(ops->mount == 0 || ops->umount == 0)
    {
        return -EINVAL;
    }

    fs = kzalloc(sizeof(vfs_fs_t));
    if(fs == 0)
    {
        return -ENOMEM;
    }

    strcpy(fs->name, name);
    fs->ops = ops;
    list_insert(&fsl, fs);

    kp_info("vfs", "registered filesystem %s", name);
    return 0;
}

void vfs_init()
{
    static vfs_ops_t ops = {0};

    static vfs_fs_t fs = {
        .ops = &ops,
    };

    static inode_t inode = {
        .flags = I_DIR,
        .fs = &fs,
        .uid = 0,
        .gid = 0,
        .mode = 0755,
        .size = 0
    };

    static dentry_t dentry = {
        .inode = &inode
    };

    if(root)
    {
        return;
    }
    else
    {
        root = &dentry;
        root->parent = root;
    }

    devfs_init();
    vfs_mount(0, "devfs", "devices");
}
