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

static inline int ext2_write_direct(ext2_t *fs, uint32_t block, void *data)
{
    kp_info("ext2", "direct write %d", block);
    block = (block * fs->sectors_per_block);
    return blkdev_write(fs->dev, block, fs->sectors_per_block, data);
}

static void *ext2_read_cached(ext2_ctx_t *ctx, uint32_t block, bool dirty)
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
            if(dirty)
            {
                item->dirty = true;
            }
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
    item->dirty = dirty;
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
    ctx->blkbuf = ctx + 1;
    list_init(&ctx->list, offsetof(ext2_blk_t, link));

    return ctx;
}

static void ext2_ctx_free(ext2_ctx_t *ctx)
{
    ext2_blk_t *item;

    while(item = list_pop(&ctx->list), item)
    {
        if(item->dirty) // if ctx->errno is set, we do not write anything
        {
            ext2_write_direct(ctx->fs, item->block, item->data); // can return an error!
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

static void ext2_inode_convert(ext2_inode_t *sp, inode_t *dp, uint32_t ino, uint32_t blksz)
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

static long ext2_inode_block(ext2_ctx_t *ctx, ext2_inode_t *inode, uint32_t offset) // use ctx->errno here as well? and size_t as return?
{
    uint32_t ident, block, *ptr;
    size_t *links;
    ext2_t *fs;

    fs = ctx->fs;
    ptr = ctx->ptr_data;
    ident = inode->block[0];

    links = ctx->ptr_links;
    links[0] = 0;
    links[1] = 0;
    links[2] = 0;

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

            if(!offset)
            {
                *links++ = block;
            }

            ptr = ext2_read_cached(ctx, block, false);
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

                if(!doubly)
                {
                    *links++ = block;
                }

                ptr = ext2_read_cached(ctx, block, false);
                if(!ptr)
                {
                    return ctx->errno;
                }

                block = ptr[doubly];
                if(block == 0)
                {
                    return 0;
                }

                if(!singly)
                {
                    *links++ = block;
                }

                ptr = ext2_read_cached(ctx, block, false);
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

                if(!triply)
                {
                    *links++ = block;
                }

                ptr = ext2_read_cached(ctx, block, false);
                if(!ptr)
                {
                    return ctx->errno;
                }

                block = ptr[triply];
                if(block == 0)
                {
                    return 0;
                }

                if(!doubly)
                {
                    *links++ = block;
                }

                ptr = ext2_read_cached(ctx, block, false);
                if(!ptr)
                {
                    return ctx->errno;
                }

                block = ptr[doubly];
                if(block == 0)
                {
                    return 0;
                }

                if(!singly)
                {
                    *links++ = block;
                }

                ptr = ext2_read_cached(ctx, block, false);
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

static ext2_bgd_t *ext2_read_bgd(ext2_ctx_t *ctx, uint32_t bg, int dirty)
{
    uint32_t block, offset;
    ext2_bgd_t *table;
    ext2_t *fs;

    fs = ctx->fs;
    block = fs->bgds_start + (bg / fs->bgds_per_block);
    offset = bg % fs->bgds_per_block;

    table = ext2_read_cached(ctx, block, dirty);
    if(!table)
    {
        return 0;
    }

    return table + offset;
}

static ext2_inode_t *ext2_read_inode(ext2_ctx_t *ctx, uint32_t ino, int dirty)
{
    uint32_t bg, block, offset;
    ext2_inode_t *table;
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    ext2_t *fs;

    fs = ctx->fs;
    sb = fs->sb;

    bg     = (ino - 1) / sb->inodes_per_group;
    ino    = (ino - 1) % sb->inodes_per_group;
    block  = (ino * sb->inode_size) / fs->block_size;
    offset = ino % fs->inodes_per_block;

    bgd = ext2_read_bgd(ctx, bg, false);
    if(!bgd)
    {
        return 0;
    }

    table = ext2_read_cached(ctx, block + bgd->inode_table, dirty);
    if(!table)
    {
        return 0;
    }

    return table + offset;
}

static int ext2_walk_directory(ext2_ctx_t *ctx, inode_t *ip, size_t seek, void *data, ext2_lookup_t *lookup)
{
    ext2_inode_t *i_dir, *i_ent;
    ext2_dentry_t *dent;
    inode_t inode;
    int count, status;
    uint8_t *ptr, *end;
    ext2_t *fs;
    long block;
    char filename[256];

    i_dir = ext2_read_inode(ctx, ip->ino, false);
    if(!i_dir)
    {
        return ctx->errno;
    }

    fs = ctx->fs;
    count = i_dir->size / fs->block_size;

    for(int i = 0; i < count; i++)
    {
        block = ext2_inode_block(ctx, i_dir, i);
        if(block < 1)
        {
            return block;
        }

        ptr = ext2_read_cached(ctx, block, false);
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

            i_ent = ext2_read_inode(ctx, dent->inode, false);
            if(!i_ent)
            {
                return ctx->errno;
            }
            ext2_inode_convert(i_ent, &inode, dent->inode, fs->block_size);

            if(data)
            {
                status = vfs_put_dirent(data, filename, &inode); // if we want to store some sort of offset, we should store the block number
                if(status < 0)
                {
                    return 0;
                }
            }
            else if(lookup)
            {
                if(strcmp(filename, lookup->name) == 0)
                {
                    if(lookup->inode)
                    {
                        lookup->inode[0] = inode;
                    }
                    lookup->dent = dent;
                    lookup->block = block;
                    return 0;
                }
            }
        }
    }

    return 0;
}

static int ext2_dirent_init(ext2_ctx_t *ctx, uint32_t ino)
{
    ext2_inode_t *ip;
    ext2_iter_t *it;
    uint32_t block;
    ext2_t *fs;
    void *ptr;
    int status;

    it = &ctx->iter;
    fs = ctx->fs;

    ip = ext2_read_inode(ctx, ino, false);
    if(!ip)
    {
        return ctx->errno;
    }

    block = ext2_inode_block(ctx, ip, 0);
    if(block < 0)
    {
        return status;
    }

    ptr = ext2_read_cached(ctx, block, false);
    if(!ptr)
    {
        return ctx->errno;
    }

    it->offset = 0;
    it->count  = ip->size / fs->block_size;
    it->block  = block;
    it->ptr    = ptr;
    it->end    = ptr + ctx->fs->block_size;
    it->ip     = ip;

    return 0;
}

static ext2_dentry_t *ext2_dirent_next(ext2_ctx_t *ctx, uint32_t *block, char *filename)
{
    uint32_t status;
    ext2_dentry_t *dp;
    ext2_iter_t *it;
    ext2_t *fs;
    void *ptr;

    fs   = ctx->fs;
    it   = &ctx->iter;
    dp   = it->ptr;

    if(!dp)
    {
        return 0;
    }

    ptr = it->ptr + dp->size;

    if(ptr >= it->end)
    {
        it->ptr = 0;
        it->offset++;

        if(it->offset < it->count)
        {
            status = ext2_inode_block(ctx, it->ip, it->offset);
            if(status < 1)
            {
                ctx->errno = status;
                return 0;
            }

            ptr = ext2_read_cached(ctx, status, false);
            if(!ptr)
            {
                return 0;
            }

            it->ptr   = ptr;
            it->end   = ptr + fs->block_size;
            it->block = status;
        }
    }
    else
    {
        it->ptr = ptr;
    }

    if(block)
    {
        *block = it->block;
    }

    if(filename)
    {
        strncpy(filename, dp->name, dp->length);
        filename[dp->length] = '\0';
    }

    return dp;
}

static int ext2_read_file(ext2_ctx_t *ctx, file_t *file, size_t size, void *buf)
{
    size_t fsize, fstart, esize;
    size_t offset, whole;
    ext2_inode_t *ip;
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

    ip = ext2_read_inode(ctx, file->inode->ino, false);
    if(!ip)
    {
        return ctx->errno;
    }

    fs = file->inode->data;
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
        block = ext2_inode_block(ctx, ip, offset);
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
        block = ext2_inode_block(ctx, ip, offset);
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
        block = ext2_inode_block(ctx, ip, offset);
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

    status = ext2_walk_directory(ctx, file->inode, seek, data, 0);
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

    ext2_lookup_t lookup = {
        .name = name,
        .inode = inode,
        .dent = 0,
        .block = 0,
    };

    status = ext2_walk_directory(ctx, ip, 0, 0, &lookup);
    ext2_ctx_free(ctx);

    if(status < 0)
    {
        return status;
    }

    if(lookup.block == 0)
    {
        return -ENOENT;
    }

    return 0;
}

static int ext2_free_blocks(ext2_ctx_t *ctx, uint32_t start, uint32_t count)
{
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    uint8_t *bitmap;
    int offset, bit;
    int bg, pos;
    int status;

    kp_info("ext2", "freeing %d blocks from %d-%d", count, start, start+count-1);

    sb  = ctx->fs->sb;
    bg  = (start - 1) / sb->blocks_per_group; // -1 might be dynamic: sb->first_data_block ?
    pos = (start - 1) % sb->blocks_per_group;

    bgd = ext2_read_bgd(ctx, bg, true);
    if(!bgd)
    {
        return ctx->errno;
    }

    bitmap = ext2_read_cached(ctx, bgd->block_bitmap, true);
    if(!bitmap)
    {
        return ctx->errno;
    }

    for(int i = 0; i < count; i++)
    {
        offset = pos / 8;
        bit = pos % 8;

        bitmap[offset] &= ~(1 << bit);
        bgd->free_blocks++;
        pos++;
    }

    // update superblock count

    return 0;
}

static int ext2_inode_truncate(ext2_ctx_t *ctx, uint32_t ino) //
{
    ext2_inode_t *ip;
    ext2_t *fs;
    int count;
    int status;
    int block, prev, start;
    size_t *links, nlinks;

    ip = ext2_read_inode(ctx, ino, true);
    if(!ip)
    {
        return ctx->errno;
    }

    fs     = ctx->fs;
    count  = (ip->sectors / fs->sectors_per_block);
    block  = ip->block[0];
    start  = block;
    prev   = block;
    links  = ctx->blkbuf;
    nlinks = 0;

    if(block == 0)
    {
        return 0;
    }

    // free data blocks
    for(int i = 1; i < count; i++)
    {
        block = ext2_inode_block(ctx, ip, i);
        if(block == 0)
        {
            count = i;
            break;
        }

        for(int i = 0; i < 3; i++)
        {
            if(ctx->ptr_links[i])
            {
                links[nlinks] = ctx->ptr_links[i];
                nlinks++;
            }
            else
            {
                break;
            }
        }

        if(block != prev + 1)
        {
            ext2_free_blocks(ctx, start, prev - start + 1);
            start = block;
        }

        prev = block;
    }

    ext2_free_blocks(ctx, start, prev - start + 1);

    // free indirect blocks
    if(nlinks)
    {
        block = links[0];
        start = block;
        prev  = block;

        for(int i = 1; i < nlinks; i++)
        {
            block = links[i];
            if(block != prev + 1)
            {
                ext2_free_blocks(ctx, start, prev - start + 1);
                start = block;
            }
            prev = block;
        }

        ext2_free_blocks(ctx, start, prev - start + 1);
    }

    // update inode
    ip->sectors = 0;
    ip->ctime = 0; // system_timestamp() ?

    for(int i = 0; i < 15; i++)
    {
        ip->block[i] = 0;
    }

    return 0;
}

static int ext2_inode_free(ext2_ctx_t *ctx, uint32_t ino)
{
    ext2_inode_t *ip;
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    int status, bg, pos;
    uint8_t *bitmap;

    ip = ext2_read_inode(ctx, ino, true);
    if(!ip)
    {
        return ctx->errno;
    }

    sb  = ctx->fs->sb;
    bg  = (ino - 1) / sb->inodes_per_group;
    pos = (ino - 1) % sb->blocks_per_group;

    bgd = ext2_read_bgd(ctx, bg, true);
    if(!bgd)
    {
        return ctx->errno;
    }

    bitmap = ext2_read_cached(ctx, bgd->inode_bitmap, true);
    if(!bitmap)
    {
        return ctx->errno;
    }

    bitmap[pos / 8] &= ~(1 << (pos % 8));
    bgd->free_inodes++;
    ip->dtime = 0;

    // update superblock count

    return 0;
}

static int ext2_dirent_free(ext2_ctx_t *ctx, uint32_t ino, const char *name)
{
    ext2_dentry_t *dent;
    uint32_t block;
    char filename[256];
    int status;

    status = ext2_dirent_init(ctx, ino);
    if(status < 0)
    {
        // something
    }
    while(dent = ext2_dirent_next(ctx, &block, filename), dent)
    {
        kp_info("ext2","???? %s",filename);
        if(strcmp(name, filename) == 0)
        {
            kp_info("ext2","found %s",filename);
        }
    }
}

static int ext2_unlink(inode_t *ip, dentry_t *dp)
{
    ext2_ctx_t *ctx;
    int status;

    kp_info("ext2", "unlinking >%s< in parent dir with ino %d", dp->name, ip->ino);

    ctx = ext2_ctx_alloc(ip->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    // need to check that the inode "links" number is 1, otherwise we only delete the dirent
    // do we allow unlinking empty directories using this function?
    // there is no easy way to determine if a directory is empty, we have to check all elements for inode = 0
    // if there is a subdirectory, then inode.links > 2 and we can skip the walk
    // when removing a directory, we need to update inode.links on the parent
    // we should merge empty dirents
ext2_dirent_free(ctx, ip->ino, dp->name);
while(1);
    // ext2_lookup_t lookup = {
    //     .name = dp->name,
    //     .inode = 0,
    //     .dent = 0,
    //     .block = 0,
    // };

    // status = ext2_walk_directory(ctx, ip, 0, 0, &lookup);
    // if(status < 0)
    // {
    //     return status;
    // }

    // if(lookup.dent == 0)
    // {
    //     return -ENOENT; // this should not happen
    // }

    // lookup.dent->inode = 0;
    // ext2_write_cached(ctx, lookup.block);

    // there are two parts to this
    // removing the dirent
    // removing the inode, if # links is zero
    // deleting the inode itself, and truncating all blocks
    ext2_inode_truncate(ctx, dp->inode->ino);
    ext2_inode_free(ctx, dp->inode->ino);

    ext2_ctx_free(ctx);
    return 0;
}

static void *ext2_mount(devfs_t *dev, inode_t *inode)
{
    int version, status;
    ext2_inode_t *root;
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

    root = ext2_read_inode(fs->ctx, 2, false);
    ext2_inode_convert(root, inode, 2, fs->block_size);

    kp_info("ext2", "version: ext%d", version);
    kp_info("ext2", "required: %#04x", sb->features_required); // 0x0002 = Directory entries contain a type field
    kp_info("ext2", "readonly: %#04x", sb->features_readonly); // 0x0003 = Sparse superblocks and group descriptor tables, File system uses a 64-bit file size
    kp_info("ext2", "optional: %#04x", sb->features_optional); // 0x0038 =  Inodes have extended attributes, File system can resize itself for larger partitions, Directories use hash index
    kp_info("ext2", "blocks: %d (%d bytes)", sb->blocks_count, fs->block_size);
    kp_info("ext2", "inodes: %d (%d bytes)", sb->inodes_count, sb->inode_size);

    ext2_ctx_free(fs->ctx);
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
        .unlink = ext2_unlink,
        .mount = ext2_mount,
        .umount = ext2_umount
    };
    vfs_register("ext2", &ops);
}
