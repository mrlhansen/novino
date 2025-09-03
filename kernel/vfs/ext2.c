#include <kernel/storage/blkdev.h>
#include <kernel/mem/heap.h>
#include <kernel/vfs/ext2.h>
#include <kernel/vfs/vfs.h>
#include <kernel/cleanup.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

DEFINE_AUTOFREE_TYPE(void)
DEFINE_AUTOFREE_TYPE(uint32_t)

static void entry_to_inode(ext2_inode_t *sp, inode_t *dp, uint32_t ino)
{
    int flags, mode;

    flags = ((sp->mode >> 12) & 0x0F);
    mode  = (sp->mode & 0x0FFF);

    dp->ino = ino;
    dp->size = sp->size;
    dp->flags = 0;
    dp->links = sp->links_count;
    dp->atime = sp->atime;
    dp->mtime = sp->ctime;
    dp->ctime = sp->mtime;
    dp->uid = sp->uid;
    dp->gid = sp->gid;
    dp->mode = mode;

    if(flags == 0x04)
    {
        dp->flags = I_DIR;
    }
    else if(flags == 0x08)
    {
        dp->flags = I_FILE;
    }
    else if(flags == 0x0A)
    {
        dp->flags = I_SYMLINK;
    }
}

static inline int ext2_read_block(ext2_t *fs, uint32_t block, void *data)
{
    block = (block * fs->sectors_per_block);
    return blkdev_read(fs->dev, block, fs->sectors_per_block, data);
}

static long ext2_inode_block(ext2_t *fs, ext2_inode_t *inode, uint32_t offset)
{
    autofree(uint32_t) *ptr = 0;
    uint32_t block;
    int status;

    long direct = 12;
    long singly = (fs->block_size / 4);
    long doubly = singly*singly;
    long triply = doubly*singly;

    ptr = kmalloc(fs->block_size);
    if(!ptr)
    {
        return -ENOMEM;
    }

    if(offset < direct)
    {
        block = inode->block[offset];
    }
    else
    {
        offset -= direct;
        if(offset < singly)
        {
            block = inode->block[12];
            if(block == 0)
            {
                return 0;
            }

            status = ext2_read_block(fs, block, ptr);
            if(status < 0)
            {
                return status;
            }

            block = ptr[offset];
        }
        else
        {
            offset -= singly;
            if(offset < doubly)
            {
                doubly = (offset / singly);
                singly = (offset % singly);

                block = inode->block[13];
                if(block == 0)
                {
                    return 0;
                }

                status = ext2_read_block(fs, block, ptr);
                if(status < 0)
                {
                    return status;
                }

                block = ptr[doubly];
                if(block == 0)
                {
                    return 0;
                }

                status = ext2_read_block(fs, block, ptr);
                if(status < 0)
                {
                    return status;
                }

                block = ptr[singly];
            }
            else
            {
                offset -= doubly;
                triply = (offset / doubly);
                doubly = ((offset % doubly) / singly);
                singly = (offset % singly);

                block = inode->block[14];
                if(block == 0)
                {
                    return 0;
                }

                status = ext2_read_block(fs, block, ptr);
                if(status < 0)
                {
                    return status;
                }

                block = ptr[triply];
                if(block == 0)
                {
                    return 0;
                }

                status = ext2_read_block(fs, block, ptr);
                if(status < 0)
                {
                    return status;
                }

                block = ptr[doubly];
                if(block == 0)
                {
                    return 0;
                }

                status = ext2_read_block(fs, block, ptr);
                if(status < 0)
                {
                    return status;
                }

                block = ptr[singly];
            }
        }
    }

    return block;
}

static ext2_bgd_t *ext2_read_bgd(ext2_t *fs, uint32_t bg, void *data) // take ext2_bgd_t as argument and store it there
{
    uint32_t block, offset;
    ext2_bgd_t *bgd = data;
    int status;

    block = fs->bgds_start + (bg / fs->bgds_per_block);
    offset = (bg % fs->bgds_per_block);
    status = ext2_read_block(fs, block, data);
    if(status < 0)
    {
        return 0;
    }

    bgd += offset;
    return bgd;
}

static int ext2_read_inode(ext2_t *fs, ext2_inode_t *ip, uint32_t inode)
{
    autofree(void) *tmpbuf = 0;
    uint32_t group, offset, block;
    ext2_bgd_t *bgd;
    int status;

    tmpbuf = kmalloc(fs->block_size);
    if(!tmpbuf)
    {
        return -ENOMEM;
    }

    group = (inode - 1) / fs->sb->inodes_per_group;
    inode = (inode - 1) % fs->sb->inodes_per_group;
    block = (inode * fs->sb->inode_size) / fs->block_size;
    inode = (inode % fs->inodes_per_block) * fs->sb->inode_size;

    bgd = ext2_read_bgd(fs, group, tmpbuf);
    if(bgd == 0)
    {
        return fs->errno;
    }

    offset = bgd->inode_table;
    status = ext2_read_block(fs, block + offset, tmpbuf);
    if(status < 0)
    {
        return status;
    }

    *ip = *(ext2_inode_t*)(tmpbuf + inode);
    return 0;
}

static int ext2_walk_directory(inode_t *ip, size_t seek, void *data, const char *lookup_name, inode_t *lookup_inode)
{
    autofree(void) *tmpbuf = 0;
    ext2_inode_t i_dir, i_ent;
    ext2_dentry_t *dent;
    inode_t inode;
    int count, status;
    uint8_t *ptr, *end;
    ext2_t *fs;
    long block;
    char filename[256];

    fs = ip->data;

    tmpbuf = kmalloc(fs->block_size);
    if(!tmpbuf)
    {
        return -ENOMEM;
    }
    ptr = tmpbuf;

    status = ext2_read_inode(fs, &i_dir, ip->ino);
    if(status < 0)
    {
        return status;
    }
    else
    {
        count = (i_dir.sectors_count / fs->sectors_per_block);
    }

    for(int i = 0; i < count; i++)
    {
        block = ext2_inode_block(fs, &i_dir, i);
        if(block < 0)
        {
            return block;
        }
        if(block == 0)
        {
            break;
        }

        ptr = tmpbuf;
        status = ext2_read_block(fs, block, ptr);
        if(status < 0)
        {
            return status;
        }
        end = ptr + fs->block_size;

        while(ptr < end)
        {
            dent = (ext2_dentry_t*)ptr;
            ptr += dent->size;

            if(dent->inode == 0)
            {
                continue;
            }

            memset(filename, 0, 256);
            strncpy(filename, dent->name, dent->length);

            if(dent->length < 3)
            {
                if(strcmp(filename, ".") == 0)
                {
                    continue;
                }
                if(strcmp(filename, "..") == 0)
                {
                    continue;
                }
            }

            if(seek)
            {
                seek--;
                continue;
            }

            status = ext2_read_inode(fs, &i_ent, dent->inode);
            if(status < 0)
            {
                return status;
            }

            entry_to_inode(&i_ent, &inode, dent->inode);
            inode.blksz = fs->block_size;
            inode.blocks = (inode.size + inode.blksz - 1) / inode.blksz;

            if(data)
            {
                status = vfs_put_dirent(data, filename, &inode);
                if(status < 0)
                {
                    return 0;
                }
            }
            else if(lookup_name)
            {
                if(strcmp(filename, lookup_name) == 0)
                {
                    *lookup_inode = inode;
                    break;
                }
            }
        }
    }

    return 0;
}

static int ext2_read(file_t *file, size_t size, void *buf)
{
    autofree(void) *tmpbuf = 0;
    ext2_inode_t ip;
    size_t fsize, fstart, esize;
    size_t offset, whole;
    ext2_t *fs;
    int status;
    long block;

    fs = file->inode->data;

    // (offset, whole) are in units of blocks
    // (fsize, fstart, esize) are in units of bytes

    if((file->seek + size) > file->inode->size) // move this to vfs?
    {
        size = file->inode->size - file->seek;
    }

    if(!size)
    {
        return 0;
    }

    offset = (file->seek / fs->block_size);
    fstart = (file->seek % fs->block_size);

    if(fstart)
    {
        fsize = (fs->block_size - fstart);
        if(size < fsize)
        {
            fsize = size;
        }
    }
    else
    {
        fsize = 0;
    }

    whole = (size - fsize) / fs->block_size;
    esize = (size - fsize) % fs->block_size;

    ext2_read_inode(fs, &ip, file->inode->ino);

    tmpbuf = kmalloc(fs->block_size);
    if(!tmpbuf)
    {
        return -ENOMEM;
    }

    if(fsize)
    {
        block = ext2_inode_block(fs, &ip, offset);
        status = ext2_read_block(fs, block, tmpbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, tmpbuf + fstart, fsize);
        buf += fsize;
        offset++;
    }

    while(whole--)
    {
        block = ext2_inode_block(fs, &ip, offset);
        status = ext2_read_block(fs, block, buf);
        if(status < 0)
        {
            return status;
        }
        buf += fs->block_size;
        offset++;
    }

    if(esize)
    {
        block = ext2_inode_block(fs, &ip, offset);
        status = ext2_read_block(fs, block, tmpbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, tmpbuf, esize);
    }

    file->seek += size;
    return size;
}

static int ext2_seek(file_t *file, ssize_t offset, int origin)
{
    inode_t *inode;
    size_t size;

    inode = file->inode;
    size = inode->size;

    switch(origin)
    {
        case SEEK_CUR:
            offset = file->seek + offset;
            break;
        case SEEK_END:
            offset = size + offset;
            break;
    }

    if(offset < 0 || offset > size)
    {
        return -EINVAL;
    }

    file->seek = offset;
    return offset;
}

static int ext2_readdir(file_t *file, size_t seek, void *data)
{
    return ext2_walk_directory(file->inode, seek, data, 0, 0);
}

static int ext2_lookup(inode_t *ip, const char *name, inode_t *inode)
{
    int status;

    inode->flags = 0;
    status = ext2_walk_directory(ip, 0, 0, name, inode);
    if(status < 0)
    {
        return status;
    }

    if(inode->flags == 0)
    {
        return -ENOENT;
    }

    return 0;
}

static void *ext2_mount(devfs_t *dev, inode_t *inode)
{
    int version, status;
    ext2_inode_t root = {0};
    ext2_sb_t *sb;
    ext2_t *fs;

    status = blkdev_open(dev);
    if(status < 0)
    {
        return 0;
    }

    sb = kmalloc(1024);
    if(!sb)
    {
        blkdev_close(dev);
        return 0;
    }

    status = blkdev_read(dev, 2, 2, sb);
    if(status < 0)
    {
        blkdev_close(dev);
        return 0;
    }

    if(sb->signature != 0xEF53)
    {
        blkdev_close(dev);
        return 0;
    }

    if(sb->features_optional & 0x04)
    {
        if(sb->features_required >= 0x0080)
        {
            version = 4;
        }
        else
        {
            version = 3;
        }
    }
    else
    {
        version = 2;
    }

    if(version > 2)
    {
        return 0;
    }

    fs = kzalloc(sizeof(ext2_t));
    if(!fs)
    {
        return 0;
    }

    fs->version = version;
    fs->block_size = (1024 << sb->log_block_size);
    fs->inodes_per_block = (fs->block_size / sb->inode_size);
    fs->sectors_per_block = (fs->block_size / 512);
    fs->block_groups_count = (sb->blocks_count + sb->blocks_per_group - 1) / sb->blocks_per_group;
    fs->bgds_per_block = (fs->block_size / 32);
    fs->bgds_start = 1 + (sb->log_block_size == 0);
    fs->sb = sb;
    fs->dev = dev;

    ext2_read_inode(fs, &root, 2); // this could in principle also fail
    entry_to_inode(&root, inode, 2);

    kp_info("ext2", "version: ext%d",version);
    kp_info("ext2", "required: %#04x", sb->features_required); // 0x0002 = Directory entries contain a type field
    kp_info("ext2", "readonly: %#04x", sb->features_readonly); // 0x0003 = Sparse superblocks and group descriptor tables, File system uses a 64-bit file size
    kp_info("ext2", "optional: %#04x", sb->features_optional); // 0x0038 =  Inodes have extended attributes, File system can resize itself for larger partitions, Directories use hash index
    kp_info("ext2", "blocks: %d (%d bytes)", sb->blocks_count, fs->block_size);
    kp_info("ext2", "inodes: %d (%d bytes)", sb->inodes_count, sb->inode_size);

    return fs;
}

static int ext2_umount(void *data)
{
    ext2_t *fs = data;
    blkdev_close(fs->dev);
    kfree(fs);
    return 0;
}

void ext2_init()
{
    static vfs_ops_t ops = {
        .open = 0,
        .close = 0,
        .read = ext2_read,
        .write = 0,
        .seek = ext2_seek,
        .ioctl = 0,
        .readdir = ext2_readdir,
        .lookup = ext2_lookup,
        .mount = ext2_mount,
        .umount = ext2_umount
    };
    vfs_register("ext2", &ops);
}
