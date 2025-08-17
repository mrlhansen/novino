#include <kernel/vfs/initrd.h>
#include <kernel/vfs/vfs.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

// use the following command to create an initrd in ustar format
// tar -c --format=ustar -f initrd.tar <files>

static void *root = 0;
static void *end = 0;

static int oct2bin(char *c, int size)
{
    int n = 0;
    for(int i = 0; i < size; i++)
    {
        if(*c >= '0' && *c <= '7')
        {
            n = (*c - '0') + (8 * n);
        }
        else if(*c == ' ' || *c == '\0')
        {
            break;
        }
        else
        {
            return 0;
        }
        c++;
    }
    return n;
}

static int path2depth(char *str)
{
    int d = 0;
    while(*str)
    {
        if(*str == '/') d++;
        str++;
    }
    if(str[-1] == '/')
    {
        str[-1] = '\0';
        d--;
    }
    return d;
}

static void tar2inode(ustar_t *tar, inode_t *inode)
{
    inode->obj = tar;

    switch(tar->typeflag)
    {
        case REGTYPE:
            inode->flags = I_FILE;
            break;
        case DIRTYPE:
            inode->flags = I_DIR;
            break;
    }

    inode->size = tar->isize;
    inode->mode = oct2bin(tar->mode, 8);
    inode->uid  = oct2bin(tar->uid, 8);
    inode->gid  = oct2bin(tar->gid, 8);
}

static int initrd_read(file_t *file, size_t size, void *buf)
{
    ustar_t *tar;
    void *pos;

    pos = file->inode->obj;
    tar = pos;

    if((file->seek + size) > tar->isize)
    {
        size = tar->isize - file->seek;
    }

    if(!size)
    {
        return 0;
    }

    memcpy(buf, pos + 512 + file->seek, size);
    file->seek += size;

    return size;
}

static int initrd_seek(file_t *file, long offset, int origin)
{
    ustar_t *tar;
    size_t fsz;

    tar = file->inode->obj;
    fsz = tar->isize;

    switch(origin)
    {
        case SEEK_CUR:
            offset = file->seek + offset;
            break;
        case SEEK_END:
            offset = fsz + offset;
            break;
    }

    if(offset < 0 || offset > fsz)
    {
        return -EINVAL;
    }

    file->seek = offset;
    return file->seek;
}

static int initrd_readdir(file_t *file, void *data)
{
    int depth, len;
    char *dirname;
    inode_t inode;
    ustar_t *tar;
    void *pos;

    if(file->inode->obj == 0)
    {
        pos = root;
        dirname = "";
        depth = 0;
        len = 0;
    }
    else
    {
        pos = file->inode->obj;
        tar = pos;
        dirname = tar->name;
        depth = 1 + path2depth(dirname);
        len = strlen(dirname);
    }

    while(pos < end)
    {
        tar = pos;
        pos += tar->offset;

        if(depth != tar->depth)
        {
            continue;
        }

        if(strncmp(tar->name, dirname, len))
        {
            continue;
        }

        tar2inode(tar, &inode);
        if(inode.flags)
        {
            vfs_filler(data, tar->basename, &inode);
        }
    }

    return 0;
}

static int initrd_lookup(inode_t *ip, const char *name, inode_t *inode)
{
    int depth, len;
    char *dirname;
    ustar_t *tar;
    void *pos;

    if(ip->obj == 0)
    {
        pos = root;
        dirname = "";
        depth = 0;
        len = 0;
    }
    else
    {
        pos = ip->obj;
        tar = pos;
        dirname = tar->name;
        depth = 1 + path2depth(dirname);
        len = strlen(dirname);
    }

    while(pos < end)
    {
        tar = pos;
        pos += tar->offset;

        if(depth != tar->depth)
        {
            continue;
        }

        if(strncmp(tar->name, dirname, len))
        {
            continue;
        }

        if(strcmp(tar->basename, name) == 0)
        {
            tar2inode(tar, inode);
            if(inode->flags)
            {
                return 0;
            }
            else
            {
                break;
            }
        }
    }

    return -ENOENT;
}

static void *initrd_mount(devfs_t *dev, inode_t *inode)
{
    inode->obj = 0;
    inode->flags = I_DIR;
    inode->mode = 0755;
    inode->uid = 0;
    inode->gid = 0;
    inode->size = 0;
    return root;
}

static int initrd_umount(void *data)
{
    return 0;
}

void initrd_init(uint64_t address, uint32_t size)
{
    uint32_t offset;
    ustar_t *tar;
    void *pos;

    if((address == 0) || (size == 0))
    {
        return;
    }

    root = (void*)address;
    end = (void*)(address + size);
    tar = root;
    pos = root;

    // check format
    if(memcmp(tar->magic, TMAGIC, TMAGLEN))
    {
        kp_warn("initrd" ,"invalid initrd format");
        return;
    }

    if(memcmp(tar->version, TVERSION, TVERSLEN))
    {
        kp_warn("initrd" ,"invalid initrd format");
        return;
    }

    // removing trailing slash from path
    // calculate path depth
    // convert size to normal integer
    // store pointer to basename
    // store offset to next entry

    while(pos < end)
    {
        tar = pos;

        // there might be zero padding at the end
        if(*tar->name == '\0')
        {
            end = tar;
            break;
        }

        size = oct2bin(tar->size, 12);
        offset = (((size + 511) / 512) + 1) * 512;

        tar->depth = path2depth(tar->name);
        tar->isize = size;
        tar->offset = offset;
        tar->basename = strrchr(tar->name, '/');

        if(tar->basename)
        {
            tar->basename++;
        }
        else
        {
            tar->basename = tar->name;
        }

        pos += offset;
    }

    // register and mount filesystem
    static vfs_ops_t ops = {
        .open = 0,
        .close = 0,
        .read = initrd_read,
        .write = 0,
        .seek = initrd_seek,
        .ioctl = 0,
        .readdir = initrd_readdir,
        .lookup = initrd_lookup,
        .mount = initrd_mount,
        .umount = initrd_umount
    };

    vfs_register("initrd", &ops);
    vfs_mount(0, "initrd", "initrd");
}
