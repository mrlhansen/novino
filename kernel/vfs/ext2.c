#include <kernel/storage/blkdev.h>
#include <kernel/mem/heap.h>
#include <kernel/vfs/ext2.h>
#include <kernel/vfs/vfs.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

static inline int ext2_read_direct(ext2_t *fs, uint32_t block, void *data)
{
    kp_info("ext2", "direct read %d", block);
    block = (block * fs->sectors_per_block);
    return blkdev_read(fs->dev, block, fs->sectors_per_block, data);
}

static void *ext2_read_cached(ext2_ctx_t *ctx, uint32_t block)
{
    ext2_blk_t *item, *curr;
    ext2_t *fs = ctx->fs;
    int status;

    // search cached blocks
    item = ctx->list.head;
    while(item)
    {
        if(item->block == block)
        {
            return item->data;
        }
        if(item->block > block)
        {
            break;
        }
        item = item->link.next;
    }

    curr = item;
    item = 0;

    // create a new block
    item = kzalloc(sizeof(ext2_blk_t) + fs->block_size);
    if(!item)
    {
        ctx->errno = -ENOMEM;
        return 0;
    }

    item->block = block;
    item->dirty = false;
    item->data  = (void*)(item+1);

    if(curr)
    {
        list_insert_before(&ctx->list, curr, item);
    }
    else
    {
        list_append(&ctx->list, item);
    }

    // read block
    status = ext2_read_direct(fs, block, item->data);
    if(status < 0)
    {
        ctx->errno = status;
        return 0;
    }

    return item->data;
}

static inline int ext2_write_direct(ext2_t *fs, uint32_t block, void *data)
{
    kp_info("ext2", "direct write %d", block);
    block = (block * fs->sectors_per_block);
    return blkdev_write(fs->dev, block, fs->sectors_per_block, data);
}

static void ext2_write_cached(ext2_ctx_t *ctx, uint32_t block)
{
    ext2_blk_t *item;

    item = ctx->list.head;
    while(item)
    {
        if(item->block >= block)
        {
            break;
        }
        item = item->link.next;
    }

    if(item->block == block)
    {
        item->dirty = true;
        return;
    }

    kp_panic("ext2", "ext2_write_cached: unknown block!");
    while(1);
}

static ext2_ctx_t *ext2_ctx_alloc(ext2_t *fs)
{
    ext2_ctx_t *ctx = fs->ctx;
    int status;

    if(ctx)
    {
        status = atomic_lock(&ctx->lock);
        if(!status)
        {
            ctx->ptr_ident = 0;
            return ctx;
        }
    }

    ctx = kzalloc(sizeof(ext2_ctx_t) + fs->block_size);
    if(!ctx)
    {
        return 0;
    }

    ctx->fs = fs;
    list_init(&ctx->list, offsetof(ext2_blk_t, link));

    ctx->ptr_ident = 0;
    ctx->ptr_block = 0;
    ctx->ptr_data  = 0;
    ctx->blkbuf    = ctx + 1;

    return ctx;
}

static void ext2_ctx_free(ext2_ctx_t *ctx)
{
    ext2_blk_t *item;

    while(item = list_pop(&ctx->list), item)
    {
        if(item->dirty)
        {
            ext2_write_direct(ctx->fs, item->block, item->data);
        }
        kfree(item);
    }

    if(ctx == ctx->fs->ctx)
    {
        atomic_unlock(&ctx->lock);
    }
    else
    {
        kfree(ctx);
    }
}

static void entry_to_inode(ext2_inode_t *sp, inode_t *dp, uint32_t ino, uint32_t blksz)
{
    int flags, mode;

    flags = ((sp->mode >> 12) & 0x0F);
    mode  = (sp->mode & 0x0FFF);

    dp->ino = ino;
    dp->size = sp->size;
    dp->flags = 0;
    dp->mode = mode;
    dp->links = sp->links;
    dp->atime = sp->atime;
    dp->mtime = sp->ctime;
    dp->ctime = sp->mtime;
    dp->uid = sp->uid;
    dp->gid = sp->gid;
    dp->blksz = blksz;
    dp->blocks = (dp->size + blksz - 1) / blksz;

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

static long ext2_inode_block(ext2_ctx_t *ctx, ext2_inode_t *inode, uint32_t offset)
{
    uint32_t ident, block;
    uint32_t *ptr;
    ext2_t *fs;

    fs = ctx->fs;
    ptr = ctx->ptr_data;
    ident = inode->block[0];

    long direct = 12;
    long singly = (fs->block_size / 4);
    long doubly = singly*singly;
    long triply = doubly*singly;

    if(offset < direct)
    {
        block = inode->block[offset];
    }
    else
    {
        offset -= direct;
        block = offset / singly;

        if(ctx->ptr_ident == ident)
        {
            if(block == ctx->ptr_block)
            {
                offset = offset % singly;
                return ptr[offset];
            }
        }

        ctx->ptr_ident = ident;
        ctx->ptr_block = block;

        if(offset < singly)
        {
            block = inode->block[12];
            if(block == 0)
            {
                return 0;
            }

            ptr = ext2_read_cached(ctx, block);
            if(!ptr)
            {
                return ctx->errno;
            }

            ctx->ptr_data = ptr;
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

                ptr = ext2_read_cached(ctx, block);
                if(!ptr)
                {
                    return ctx->errno;
                }

                block = ptr[doubly];
                if(block == 0)
                {
                    return 0;
                }

                ptr = ext2_read_cached(ctx, block);
                if(!ptr)
                {
                    return ctx->errno;
                }

                ctx->ptr_data = ptr;
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

                ptr = ext2_read_cached(ctx, block);
                if(!ptr)
                {
                    return ctx->errno;
                }

                block = ptr[triply];
                if(block == 0)
                {
                    return 0;
                }

                ptr = ext2_read_cached(ctx, block);
                if(!ptr)
                {
                    return ctx->errno;
                }

                block = ptr[doubly];
                if(block == 0)
                {
                    return 0;
                }

                ptr = ext2_read_cached(ctx, block);
                if(!ptr)
                {
                    return ctx->errno;
                }

                ctx->ptr_data = ptr;
                block = ptr[singly];
            }
        }
    }

    return block;
}

static int ext2_read_bgd(ext2_ctx_t *ctx, uint32_t bg, ext2_bgd_t *bgd)
{
    uint32_t block, offset;
    ext2_bgd_t *table;
    ext2_t *fs;

    fs = ctx->fs;
    block = fs->bgds_start + (bg / fs->bgds_per_block);
    offset = bg % fs->bgds_per_block;

    table = ext2_read_cached(ctx, block);
    if(!table)
    {
        return ctx->errno;
    }

    *bgd = table[offset];
    return 0;
}

static int ext2_read_inode(ext2_ctx_t *ctx, ext2_inode_t *ip, uint32_t inode)
{
    uint32_t group, block, offset;
    ext2_inode_t *table;
    ext2_bgd_t bgd;
    ext2_sb_t *sb;
    ext2_t *fs;
    int status;

    fs = ctx->fs;
    sb = fs->sb;

    group  = (inode - 1) / sb->inodes_per_group;
    inode  = (inode - 1) % sb->inodes_per_group;
    block  = (inode * sb->inode_size) / fs->block_size;
    offset = inode % fs->inodes_per_block;

    status = ext2_read_bgd(ctx, group, &bgd);
    if(status < 0)
    {
        return status;
    }

    table = ext2_read_cached(ctx, block + bgd.inode_table);
    if(!table)
    {
        return ctx->errno;
    }

    *ip = table[offset];
    return 0;
}

static int ext2_walk_directory(ext2_ctx_t *ctx, inode_t *ip, size_t seek, void *data, const char *lookup_name, inode_t *lookup_inode)
{
    ext2_inode_t i_dir, i_ent;
    ext2_dentry_t *dent;
    inode_t inode;
    int count, status;
    uint8_t *ptr, *end;
    ext2_t *fs;
    long block;
    char filename[256];

    fs = ctx->fs;
    status = ext2_read_inode(ctx, &i_dir, ip->ino);
    if(status < 0)
    {
        return status;
    }
    count = (i_dir.sectors / fs->sectors_per_block);

    for(int i = 0; i < count; i++)
    {
        block = ext2_inode_block(ctx, &i_dir, i);
        if(block < 1)
        {
            return block;
        }

        ptr = ext2_read_cached(ctx, block);
        if(!ptr)
        {
            return ctx->errno;
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

            status = ext2_read_inode(ctx, &i_ent, dent->inode);
            if(status < 0)
            {
                return status;
            }
            entry_to_inode(&i_ent, &inode, dent->inode, fs->block_size);

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

static int ext2_read_file(ext2_ctx_t *ctx, file_t *file, size_t size, void *buf)
{
    size_t fsize, fstart, esize;
    size_t offset, whole;
    ext2_inode_t ip;
    ext2_t *fs;
    void *blkbuf;
    int status;
    long block;

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

    fs = file->inode->data;
    ext2_read_inode(ctx, &ip, file->inode->ino);

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
    blkbuf = ctx->blkbuf;

    if(fsize)
    {
        block = ext2_inode_block(ctx, &ip, offset);
        status = ext2_read_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, blkbuf + fstart, fsize);
        buf += fsize;
        offset++;
    }

    while(whole--)
    {
        block = ext2_inode_block(ctx, &ip, offset);
        status = ext2_read_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, blkbuf, fs->block_size);
        buf += fs->block_size;
        offset++;
    }

    if(esize)
    {
        block = ext2_inode_block(ctx, &ip, offset);
        status = ext2_read_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, blkbuf, esize);
    }

    file->seek += size; // move to vfs?
    return size;
}

static int ext2_read(file_t *file, size_t size, void *buf)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(file->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_read_file(ctx, file, size, buf);
    ext2_ctx_free(ctx);

    return status;
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
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(file->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_walk_directory(ctx, file->inode, seek, data, 0, 0);
    ext2_ctx_free(ctx);

    return status;
}

static int ext2_lookup(inode_t *ip, const char *name, inode_t *inode)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(ip->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    inode->flags = 0;
    status = ext2_walk_directory(ctx, ip, 0, 0, name, inode);
    ext2_ctx_free(ctx);

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

    fs = kzalloc(sizeof(ext2_t) + 1024);
    if(!fs)
    {
        kfree(fs);
        blkdev_close(dev);
        return 0;
    }
    sb = (void*)(fs+1);

    status = blkdev_read(dev, 2, 2, sb);
    if(status < 0)
    {
        kfree(fs);
        blkdev_close(dev);
        return 0;
    }

    if(sb->signature != 0xEF53)
    {
        kfree(fs);
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
        kfree(fs);
        blkdev_close(dev);
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
    fs->ctx = ext2_ctx_alloc(fs);

    ext2_read_inode(fs->ctx, &root, 2);
    entry_to_inode(&root, inode, 2, fs->block_size);

    kp_info("ext2", "version: ext%d", version);
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
    kfree(fs->ctx);
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
