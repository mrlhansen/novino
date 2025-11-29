#include <kernel/storage/blkdev.h>
#include <kernel/time/time.h>
#include <kernel/mem/heap.h>
#include <kernel/vfs/ext2.h>
#include <kernel/vfs/vfs.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

#define align_size(s,a)  ((s + a - 1) & -(a))

//
// Context caching, reading and writing of blocks
//

static inline int ext2_read_direct(ext2_t *fs, uint32_t block, void *data)
{
    block = (block * fs->sectors_per_block);
    return blkdev_read(fs->dev, block, fs->sectors_per_block, data);
}

static inline int ext2_write_direct(ext2_t *fs, uint32_t block, void *data)
{
    block = (block * fs->sectors_per_block);
    return blkdev_write(fs->dev, block, fs->sectors_per_block, data);
}

static void *ext2_lookup_cached(ext2_ctx_t *ctx, uint32_t block, bool create)
{
    ext2_blk_t *item;
    ext2_blk_t *curr;

    // search cached blocks
    item = ctx->list.head;
    while(item)
    {
        if(item->block == block)
        {
            return item;
        }
        if(item->block > block)
        {
            break;
        }
        item = item->link.next;
    }

    // block was not found
    if(!create)
    {
        return 0;
    }

    curr = item;
    item = 0;

    // create a new block
    item = kzalloc(sizeof(ext2_blk_t) + ctx->fs->block_size);
    if(!item)
    {
        ctx->errno = -ENOMEM;
        return 0;
    }

    item->block = block;
    item->dirty = false;
    item->ready = false;
    item->data  = (void*)(item+1);

    if(curr)
    {
        list_insert_before(&ctx->list, curr, item);
    }
    else
    {
        list_append(&ctx->list, item);
    }

    return item;
}

static void *ext2_read_cached(ext2_ctx_t *ctx, uint32_t block, bool dirty)
{
    ext2_blk_t *item;
    int status;

    // lookup in cache
    item = ext2_lookup_cached(ctx, block, true);
    if(!item)
    {
        return 0;
    }

    if(dirty)
    {
        item->dirty = true;
    }

    if(item->ready)
    {
        return item->data;
    }

    // read block
    status = ext2_read_direct(ctx->fs, block, item->data);
    if(status < 0)
    {
        ctx->errno = status;
        return 0;
    }

    item->ready = true;
    return item->data;
}

static void ext2_write_cached(ext2_ctx_t *ctx, uint32_t block)
{
    ext2_blk_t *item;
    item = ext2_lookup_cached(ctx, block, false);
    if(item)
    {
        item->dirty = true;
    }
}

static void *ext2_init_cached(ext2_ctx_t *ctx, uint32_t block)
{
    ext2_blk_t *item;

    item = ext2_lookup_cached(ctx, block, true);
    if(!item)
    {
        return 0;
    }

    if(item->ready)
    {
        memset(item->data, 0, ctx->fs->block_size);
    }

    item->ready = true;
    item->dirty = true;

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
            ctx->errno = 0;
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

static void ext2_ctx_free(ext2_ctx_t *ctx, int status)
{
    ext2_blk_t *item;
    bool flush;

    if(status < 0 || ctx->errno < 0)
    {
        flush = false;
    }
    else
    {
        flush = true;
    }

    while(item = list_pop(&ctx->list), item)
    {
        if(flush && item->dirty)
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

//
// Inodes, blocks, and block group descriptors
//

static ext2_bgd_t *ext2_bgd_read(ext2_ctx_t *ctx, uint32_t bg, bool dirty)
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

// search for the most empty bgd
// this function might need to be rewritten in the future, we might not want to search through every single block
// the returned bg must have at least one free inode and one free block
// maybe we should have flags to require free inodes or free blocks?
static int ext2_bgd_search(ext2_ctx_t *ctx, uint32_t *bg)
{
    ext2_bgd_t *bgd;
    ext2_t *fs;
    uint32_t blocks;

    fs = ctx->fs;
    blocks = 0;

    for(int i = 0; i < fs->bgds_total; i++)
    {
        bgd = ext2_bgd_read(ctx, i, false);
        if(!bgd)
        {
            return ctx->errno;
        }
        if(!bgd->free_inodes) // running out of inodes before running out of blocks is not really possible unless we have a lot of empty files?
        {
            continue;
        }
        if(bgd->free_blocks > blocks)
        {
            blocks = bgd->free_blocks;
            *bg = i;
        }
    }

    if(!blocks)
    {
        return -ENOSPC;
    }

    return 0;
}

static int ext2_blocks_free(ext2_ctx_t *ctx, uint32_t start, uint32_t count)
{
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    uint8_t *bitmap;
    int bg, pos;

    sb  = ctx->fs->sb;
    pos = start - sb->first_data_block;
    bg  = pos / sb->blocks_per_group;
    pos = pos % sb->blocks_per_group;

    bgd = ext2_bgd_read(ctx, bg, true);
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
        bitmap[pos/8] &= ~(1 << (pos%8));
        pos++;
    }

    bgd->free_blocks += count;
    sb->free_blocks += count;

    return 0;
}

// we update bg with the block group where the blocks are, initial bg is our wish
// we update count with the remaining blocks we were not able to allocate
// in this way we can keep calling the function until count = 0
static int ext2_blocks_alloc(ext2_ctx_t *ctx, uint32_t *bg, uint32_t *count, uint32_t *start)
{
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    ext2_t *fs;
    uint32_t grp, cnt;
    uint8_t *bitmap;
    int status;

    fs = ctx->fs;
    sb = fs->sb;
    grp = *bg;
    cnt = *count;

    if(!cnt)
    {
        return 0;
    }

    bgd = ext2_bgd_read(ctx, grp, true);
    if(!bgd)
    {
        return ctx->errno;
    }

    if(!bgd->free_blocks)
    {
        status = ext2_bgd_search(ctx, &grp);
        if(status < 0)
        {
            return status;
        }

        bgd = ext2_bgd_read(ctx, grp, true);
        if(!bgd)
        {
            return ctx->errno;
        }
    }

    bitmap = ext2_read_cached(ctx, bgd->block_bitmap, true);
    if(!bitmap)
    {
        return ctx->errno;
    }

    // best fit search for a free range of blocks
    uint32_t curr_start = 0;
    uint32_t curr_count = 0;
    uint32_t best_start = 0;
    uint32_t best_count = 0;
    uint8_t bit = 0;

    for(int i = 0; i < fs->block_size; i++)
    {
        if(bitmap[i] == 0xFF)
        {
            continue;
        }

        for(int j = 0; j < 8; j++)
        {
            bit = bitmap[i] & (1 << j);

            if(!bit)
            {
                if(!curr_count)
                {
                    curr_start = 8*i+j;
                    curr_count = 1;
                }
                else
                {
                    curr_count++;
                }
                continue;
            }

            if(!curr_count)
            {
                continue;
            }

            if(!best_count)
            {
                best_start = curr_start;
                best_count = curr_count;
            }
            else if(curr_count >= cnt)
            {
                if((best_count < cnt) || (curr_count < best_count))
                {
                    best_start = curr_start;
                    best_count = curr_count;
                }
            }
            else if(curr_count > best_count)
            {
                best_start = curr_start;
                best_count = curr_count;
            }

            curr_count = 0;
        }

        if(best_count == cnt)
        {
            break;
        }
    }

    if(!best_count)
    {
        best_start = curr_start;
        best_count = curr_count;
    }
    else if(curr_count >= cnt)
    {
        if((best_count < cnt) || (curr_count < best_count))
        {
            best_start = curr_start;
            best_count = curr_count;
        }
    }
    else if(curr_count > best_count)
    {
        best_start = curr_start;
        best_count = curr_count;
    }

    // allocate blocks
    uint32_t pos = best_start;
    uint32_t max = best_count;

    if(max > cnt)
    {
        max = cnt;
    }

    for(int i = 0; i < max; i++)
    {
        bitmap[pos/8] |= (1 << (pos%8));
        pos++;
    }

    // update variables
    *start = best_start + (grp * sb->blocks_per_group);
    *count = cnt - max;
    *bg = grp;

    bgd->free_blocks -= max;
    sb->free_blocks -= max;

    return max;
}

static void ext2_blocks_dist(ext2_t *fs, ext2_ibd_t *item, uint32_t nblocks, bool inclusive)
{
    int singly, doubly;
    ssize_t count;

    item->ns = 0;
    item->nd = 0;
    item->nds = 0;
    item->nt = 0;
    item->ntd = 0;
    item->nts = 0;

    singly = (fs->block_size / 4);
    doubly = singly * singly;
    count  = nblocks;

    if(count > 12)
    {
        count = count - 12;
        item->ns = 1;

        if(inclusive)
        {
            count = count - item->ns;
        }

        if(count > singly)
        {
            count = count - singly;
            item->nd = 1;

            if(inclusive)
            {
                count = count - item->nd;
            }

            if(count > doubly)
            {
                count = count - doubly;
                item->nds = singly;
                item->nt = 1;

                if(inclusive)
                {
                    count = count - item->nds - item->nt;
                }

                while(count > 0)
                {
                    if((item->nts % singly) == 0)
                    {
                        item->nds++;
                        if(inclusive) count--;
                    }
                    item->nts++;
                    if(inclusive) count--;
                    count = count - singly;
                }
            }
            else
            {
                while(count > 0)
                {
                    item->nds++;
                    if(inclusive) count--;
                    count = count - singly;
                }
            }
        }
    }

    item->inum = item->ns + item->nd + item->nds + item->nt + item->ntd + item->nts;

    if(inclusive)
    {
        item->dnum = nblocks - item->inum;
    }
    else
    {
        item->dnum = nblocks;
    }

}

static void ext2_inode_settime(ext2_inode_t *dp, int flags)
{
    timeval_t tv;
    gettime(&tv);

    if(flags & EXT2_ATIME)
    {
        dp->atime = tv.tv_sec;
        dp->ctime = tv.tv_sec;
    }

    if(flags & EXT2_CTIME)
    {
        dp->ctime = tv.tv_sec;
    }

    if(flags & EXT2_DTIME)
    {
        dp->dtime = tv.tv_sec;
        dp->ctime = tv.tv_sec;
    }

    if(flags & EXT2_MTIME)
    {
        dp->mtime = tv.tv_sec;
        dp->ctime = tv.tv_sec;
    }
}

static void ext2_inode_setattr(ext2_inode_t *dp, inode_t *sp)
{
    dp->mode  = sp->mode;
    dp->atime = sp->atime;
    dp->ctime = sp->ctime;
    dp->mtime = sp->mtime;
    dp->uid   = sp->uid;
    dp->gid   = sp->gid;

    if(sp->flags & I_DIR)
    {
        dp->mode |= 0x4000;
    }
    else if(sp->flags & I_FILE)
    {
        dp->mode |= 0x8000;
    }
    else if(sp->flags & I_SYMLINK)
    {
        dp->mode |= 0xA000;
    }
}

static void ext2_inode_getattr(ext2_t *fs, inode_t *dp, ext2_inode_t *sp, uint32_t ino)
{
    int flags, mode;

    flags = (sp->mode & 0xF000) >> 12;
    mode  = (sp->mode & 0x0FFF);

    dp->size   = sp->size;
    dp->flags  = 0;
    dp->mode   = mode;
    dp->links  = sp->links;
    dp->atime  = sp->atime;
    dp->ctime  = sp->ctime;
    dp->mtime  = sp->mtime;
    dp->uid    = sp->uid;
    dp->gid    = sp->gid;
    dp->blksz  = fs->block_size;
    dp->blocks = sp->sectors / fs->sectors_per_block;

    if(ino)
    {
        dp->ino = ino;
    }

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

static uint32_t ext2_inode_direct_block(ext2_ctx_t *ctx, ext2_inode_t *inode, uint32_t offset, ext2_ibw_t *ibw)
{
    uint32_t next;

    next = inode->block[offset];
    if(!next)
    {
        if(ibw->create)
        {
            if(offset < 12)
            {
                next = ibw->db_next;
                ibw->db_next++;
                ibw->db_count--;
            }
            else
            {
                next = ibw->ib_next;
                ibw->ib_next++;
                ibw->ib_count--;
                ext2_init_cached(ctx, next);
                kp_info("ext2", "linked new direct block at offset %d -> %d", offset, next);
            }
            inode->block[offset] = next;
        }
    }

    return next;
}

static void *ext2_inode_indirect_block(ext2_ctx_t *ctx, uint32_t block, uint32_t offset, ext2_ibw_t *ibw, bool data)
{
    uint32_t *ptr;
    uint32_t next;

    ptr = ext2_read_cached(ctx, block, false);
    if(!ptr)
    {
        return 0;
    }

    next = ptr[offset];
    if(!next)
    {
        if(ibw->create)
        {
            if(data)
            {
                next = ibw->db_next;
                ibw->db_next++;
                ibw->db_count--;
                kp_info("ext2", "linked new data block %d[%d] -> %d", block, offset, next);
            }
            else
            {
                next = ibw->ib_next;
                ibw->ib_next++;
                ibw->ib_count--;
                ext2_init_cached(ctx, next);
                kp_info("ext2", "linked new indirect block %d[%d] -> %d", block, offset, next);
            }
            ptr[offset] = next;
            ext2_write_cached(ctx, block);
        }
    }

    return ptr;
}

static uint32_t ext2_inode_set_block(ext2_ctx_t *ctx, ext2_inode_t *inode, uint32_t offset, ext2_ibw_t *ibw)
{
    uint32_t ident, block, index, *ptr;
    size_t *links;
    ext2_t *fs;

    fs = ctx->fs;
    ptr = ctx->ptr_data;
    ident = inode->block[0];

    links = ctx->ptr_links;
    links[0] = 0;
    links[1] = 0;
    links[2] = 0;

    size_t direct = 12;
    size_t singly = (fs->block_size / 4);
    size_t doubly = singly*singly;
    size_t triply = doubly*singly;

    if(offset < direct)
    {
        block = ext2_inode_direct_block(ctx, inode, offset, ibw);
    }
    else
    {
        offset -= direct;
        index = offset / singly;

        if(ctx->ptr_ident == ident)
        {
            if(ctx->ptr_index == index)
            {
                offset = offset % singly;
                block = ptr[offset];
                if(!block)
                {
                    if(ibw->create)
                    {
                        block = ibw->db_next;
                        ibw->db_next++;
                        ibw->db_count--;
                        ptr[offset] = block;
                        ext2_write_cached(ctx, ctx->ptr_block);
                    }
                }
                return block;
            }
        }

        ctx->ptr_ident = ident;
        ctx->ptr_index = index;

        if(offset < singly)
        {
            block = ext2_inode_direct_block(ctx, inode, 12, ibw);
            if(block == 0)
            {
                return 0;
            }

            if(!offset)
            {
                *links++ = block;
            }

            ptr = ext2_inode_indirect_block(ctx, block, offset, ibw, true);
            if(!ptr)
            {
                return 0;
            }

            ctx->ptr_data = ptr;
            ctx->ptr_block = block;
            block = ptr[offset];
        }
        else
        {
            offset -= singly;
            if(offset < doubly)
            {
                doubly = (offset / singly);
                singly = (offset % singly);

                block = ext2_inode_direct_block(ctx, inode, 13, ibw);
                if(block == 0)
                {
                    return 0;
                }

                if(!doubly)
                {
                    *links++ = block;
                }

                ptr = ext2_inode_indirect_block(ctx, block, doubly, ibw, false);
                if(!ptr)
                {
                    return 0;
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

                ptr = ext2_inode_indirect_block(ctx, block, singly, ibw, true);
                if(!ptr)
                {
                    return 0;
                }

                ctx->ptr_data = ptr;
                ctx->ptr_block = block;
                block = ptr[singly];
            }
            else
            {
                offset -= doubly;
                triply = (offset / doubly);
                doubly = ((offset % doubly) / singly);
                singly = (offset % singly);

                block = ext2_inode_direct_block(ctx, inode, 14, ibw);
                if(block == 0)
                {
                    return 0;
                }

                if(!triply)
                {
                    *links++ = block;
                }

                ptr = ext2_inode_indirect_block(ctx, block, triply, ibw, false);
                if(!ptr)
                {
                    return 0;
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

                ptr = ext2_inode_indirect_block(ctx, block, doubly, ibw, false);
                if(!ptr)
                {
                    return 0;
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

                ptr = ext2_inode_indirect_block(ctx, block, singly, ibw, true);
                if(!ptr)
                {
                    return 0;
                }

                ctx->ptr_data = ptr;
                ctx->ptr_block = block;
                block = ptr[singly];
            }
        }
    }

    return block;
}

static uint32_t ext2_inode_get_block(ext2_ctx_t *ctx, ext2_inode_t *inode, uint32_t offset)
{
    static ext2_ibw_t ibw = {
        .create = false,
    };
    return ext2_inode_set_block(ctx, inode, offset, &ibw);
}

static ext2_inode_t *ext2_inode_read(ext2_ctx_t *ctx, uint32_t ino, bool dirty)
{
    uint32_t bg, block, offset;
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    ext2_t *fs;
    void *ptr;

    fs = ctx->fs;
    sb = fs->sb;

    bg     = (ino - 1) / sb->inodes_per_group;
    ino    = (ino - 1) % sb->inodes_per_group;
    block  = (ino * sb->inode_size) / fs->block_size;
    offset = (ino * sb->inode_size) % fs->block_size;

    bgd = ext2_bgd_read(ctx, bg, false);
    if(!bgd)
    {
        return 0;
    }

    ptr = ext2_read_cached(ctx, block + bgd->inode_table, dirty);
    if(!ptr)
    {
        return 0;
    }

    return ptr + offset;
}

static int ext2_inode_free(ext2_ctx_t *ctx, uint32_t ino)
{
    ext2_inode_t *inode;
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    uint8_t *bitmap;
    int bg, pos;

    inode = ext2_inode_read(ctx, ino, true);
    if(!inode)
    {
        return ctx->errno;
    }

    sb  = ctx->fs->sb;
    bg  = (ino - 1) / sb->inodes_per_group;
    pos = (ino - 1) % sb->blocks_per_group;

    bgd = ext2_bgd_read(ctx, bg, true);
    if(!bgd)
    {
        return ctx->errno;
    }

    bitmap = ext2_read_cached(ctx, bgd->inode_bitmap, true);
    if(!bitmap)
    {
        return ctx->errno;
    }

    bitmap[pos/8] &= ~(1 << (pos%8));
    bgd->free_inodes++;
    sb->free_inodes++;
    ext2_inode_settime(inode, EXT2_DTIME);

    return 0;
}

static ext2_inode_t *ext2_inode_alloc(ext2_ctx_t *ctx, uint32_t bg, int links, inode_t *ip)
{
    ext2_inode_t *inode;
    ext2_bgd_t *bgd;
    ext2_sb_t *sb;
    ext2_t *fs;
    uint8_t *bitmap;
    int status;
    int val;

    fs = ctx->fs;
    sb = fs->sb;
    val = 0;

    bgd = ext2_bgd_read(ctx, bg, true);
    if(!bgd)
    {
        return 0;
    }

    if(!bgd->free_inodes)
    {
        kp_info("ext2", "block %d has run out of inodes!", bg);

        status = ext2_bgd_search(ctx, &bg);
        if(status < 0)
        {
            ctx->errno = status;
            return 0;
        }

        bgd = ext2_bgd_read(ctx, bg, true);
        if(!bgd)
        {
            return 0;
        }
    }

    bitmap = ext2_read_cached(ctx, bgd->inode_bitmap, true);
    if(!bitmap)
    {
        return 0;
    }

    for(int i = 0; i < fs->block_size; i++)
    {
        if(bitmap[i] == 0xFF)
        {
            continue;
        }

        val = ~bitmap[i];
        val = __builtin_ctz(val);
        bitmap[i] |= (1 << val);
        val = val + 8*i;

        break;
    }

    bgd->free_inodes--;
    sb->free_inodes--;
    ip->ino = 1 + val + (bg * sb->inodes_per_group);

    inode = ext2_inode_read(ctx, ip->ino, true);
    if(!inode)
    {
        return 0;
    }

    memset(inode, 0, sb->inode_size);
    inode->links = links;
    ext2_inode_setattr(inode, ip);

    kp_info("ext2","allocated ino %d = %d",val,ip->ino);
    return inode;
}

static int ext2_inode_truncate(ext2_ctx_t *ctx, uint32_t ino)
{
    ext2_inode_t *inode;
    ext2_ibd_t ibd;
    ext2_t *fs;
    uint32_t *links, nlinks;
    int start, count;
    int block, prev;

    inode = ext2_inode_read(ctx, ino, true);
    if(!inode)
    {
        return ctx->errno;
    }

    fs     = ctx->fs;
    count  = inode->sectors / fs->sectors_per_block;
    block  = inode->block[0];
    start  = block;
    prev   = block;
    links  = ctx->blkbuf; // TODO: we assume they will fit here
    nlinks = 0;

    if(!block)
    {
        return 0;
    }

    ext2_blocks_dist(fs, &ibd, count, true);

    // free data blocks
    for(int i = 1; i < ibd.dnum; i++)
    {
        block = ext2_inode_get_block(ctx, inode, i);
        if(!block)
        {
            kp_error("ext2", "ino %d: block not found at offset %d", ino, i);
            return -EIO;
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
            ext2_blocks_free(ctx, start, prev - start + 1);
            start = block;
        }

        prev = block;
    }

    ext2_blocks_free(ctx, start, prev - start + 1);

    // free indirect blocks
    if(nlinks != ibd.inum)
    {
        kp_error("ext2", "ino %d: indirect block count mismatch (%d vs %d)", ino, nlinks, ibd.inum);
        return -EIO;
    }

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
                ext2_blocks_free(ctx, start, prev - start + 1);
                start = block;
            }
            prev = block;
        }

        ext2_blocks_free(ctx, start, prev - start + 1);
    }

    // update inode
    ext2_inode_settime(inode, EXT2_MTIME);
    inode->size = 0;
    inode->sectors = 0;

    for(int i = 0; i < 15; i++)
    {
        inode->block[i] = 0;
    }

    return 0;
}

static int ext2_inode_expand(ext2_ctx_t *ctx, uint32_t ino, uint32_t goal, bool grow)
{
    ext2_inode_t *inode;
    ext2_ibd_t a, b;
    ext2_sb_t *sb;
    ext2_t *fs;
    uint32_t bg, count;
    uint32_t total, start;
    int status;

    inode = ext2_inode_read(ctx, ino, true);
    if(!inode)
    {
        return ctx->errno;
    }

    fs = ctx->fs;
    sb = fs->sb;
    bg = (ino - 1) / sb->inodes_per_group;
    count = inode->sectors / fs->sectors_per_block;

    // block distribution before (b) and after (a) expansion
    ext2_blocks_dist(fs, &b, count, true);
    ext2_blocks_dist(fs, &a, goal, false);

    if(a.dnum <= b.dnum)
    {
        return 0;
    }

    total = a.inum - b.inum;
    start = 0;

    // Let's assume we can find a full range for the indirect blocks for now
    // If not, it's unlikely we can find enough data blocks
    status = ext2_blocks_alloc(ctx, &bg, &total, &start);
    if(status < 0)
    {
        return status;
    }

    if(total)
    {
        return -ENOSPC;
    }

    ext2_ibw_t ibw = {
        .create = true,
        .ib_count = status,
        .ib_next = start,
        .db_count = 0,
        .db_next = 0,
    };

    total = a.dnum - b.dnum;
    start = 0;

    for(int i = count; i < goal; i++)
    {
        if(ibw.db_count == 0)
        {
            status = ext2_blocks_alloc(ctx, &bg, &total, &start);
            if(status < 0)
            {
                return status;
            }
            ibw.db_count = status;
            ibw.db_next = start;
        }
        ext2_inode_set_block(ctx, inode, i, &ibw);
    }

    inode->sectors = (a.inum + a.dnum) * fs->sectors_per_block;
    if(grow)
    {
        inode->size = a.dnum * fs->block_size;
    }

    return 0;
}

//
// Directory entries (allocating, freeing, iterating)
//

static ext2_dentry_t *ext2_dirent_next(ext2_ctx_t *ctx, uint32_t *block, char *filename)
{
    uint32_t status;
    ext2_dentry_t *dp;
    ext2_iter_t *it;
    ext2_t *fs;
    void *ptr;

    fs = ctx->fs;
    it = ctx->iter;
    dp = it->ptr;

    if(!dp)
    {
        return 0;
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

    it->ptr += dp->size;

    if(it->ptr >= it->end)
    {
        it->ptr = 0;
        it->offset++;

        if(it->offset < it->count)
        {
            status = ext2_inode_get_block(ctx, it->ip, it->offset);
            if(!status)
            {
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

    return dp;
}

static int ext2_dirent_iter(ext2_ctx_t *ctx, uint32_t ino, bool nodots)
{
    ext2_inode_t *ip;
    ext2_iter_t *it;
    uint32_t block;
    ext2_t *fs;
    void *ptr;

    it = ctx->iter;
    fs = ctx->fs;

    ip = ext2_inode_read(ctx, ino, false);
    if(!ip)
    {
        return ctx->errno;
    }

    block = ext2_inode_get_block(ctx, ip, 0);
    if(!block)
    {
        return ctx->errno;
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
    it->end    = ptr + fs->block_size;
    it->ip     = ip;

    // skip . and .. entries
    if(nodots)
    {
        ext2_dirent_next(ctx, 0, 0);
        ext2_dirent_next(ctx, 0, 0);
    }

    return 0;
}

static int ext2_dirent_walk(ext2_ctx_t *ctx, uint32_t ino, size_t seek, void *data, const char *lookup_name, inode_t *lookup_inode)
{
    ext2_dentry_t *dp;
    ext2_inode_t *ip;
    inode_t inode;
    int status;
    char filename[256];

    status = ext2_dirent_iter(ctx, ino, true);
    if(status < 0)
    {
        return status;
    }

    while(dp = ext2_dirent_next(ctx, 0, filename), dp)
    {
        if(!dp)
        {
            break;
        }

        if(!dp->inode)
        {
            continue;
        }

        if(seek)
        {
            seek--;
            continue;
        }

        ip = ext2_inode_read(ctx, dp->inode, false);
        if(!ip)
        {
            break;
        }
        ext2_inode_getattr(ctx->fs, &inode, ip, dp->inode);

        if(data)
        {
            status = vfs_put_dirent(data, filename, &inode);
            if(status < 0)
            {
                break;
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

    return ctx->errno;
}

static int ext2_dirent_unlink(ext2_ctx_t *ctx, inode_t *dir, const char *name, inode_t *obj)
{
    ext2_dentry_t *dp, *prev;
    ext2_inode_t *inode;
    uint32_t cblk, pblk;
    char filename[256];
    int status;

    // initialize iterator
    status = ext2_dirent_iter(ctx, dir->ino, true);
    if(status < 0)
    {
        return status;
    }

    // search for entry
    pblk = 0;
    prev = 0;

    while(dp = ext2_dirent_next(ctx, &cblk, filename), dp)
    {
        if(strcmp(name, filename) == 0)
        {
            break;
        }
        pblk = cblk;
        prev = dp;
    }

    if(!dp)
    {
        return -ENOENT;
    }

    // merge with previous if within same block, otherwise mark as unused
    if(pblk == cblk)
    {
        prev->size += dp->size;
    }
    else
    {
        dp->inode = 0;
        pblk = cblk;
        prev = dp;
    }

    // also merge next entry if within block and free
    dp = ext2_dirent_next(ctx, &cblk, filename);
    if(dp)
    {
        if(pblk == cblk && dp->inode == 0)
        {
            prev->size += dp->size;
        }
    }

    // mark as dirty
    ext2_write_cached(ctx, pblk);

    // adjust parent inode
    inode = ext2_inode_read(ctx, dir->ino, true);
    ext2_inode_settime(inode, EXT2_MTIME);

    if(obj->flags & I_DIR)
    {
        inode->links--;
    }

    ext2_inode_getattr(ctx->fs, dir, inode, 0);

    return 0;
}

static int ext2_dirent_link(ext2_ctx_t *ctx, inode_t *dir, const char *name, inode_t *obj)
{
    ext2_inode_t *inode;
    ext2_dentry_t *dp;
    ext2_sb_t *sb;
    ext2_t *fs;
    uint32_t block;
    int status, count;
    int req, used, avail;

    fs  = ctx->fs;
    sb = fs->sb;
    req = sizeof(ext2_dentry_t) + strlen(name);
    req = align_size(req, 4);

    // read parent inode
    inode = ext2_inode_read(ctx, dir->ino, true);
    if(!inode)
    {
        return ctx->errno;
    }

    // initialize iterator
    status = ext2_dirent_iter(ctx, dir->ino, false);
    if(status < 0)
    {
        return status;
    }

    // look for empty slot
    while(dp = ext2_dirent_next(ctx, &block, 0), dp)
    {
        used = 0;
        if(dp->inode)
        {
            used = sizeof(ext2_dentry_t) + dp->length;
            used = align_size(used, 4);
        }
        avail = dp->size - used;

        if(avail > req)
        {
            if(dp->inode)
            {
                dp->size = used;
                dp = (void*)dp + dp->size;
                dp->size = avail;
            }

            ext2_write_cached(ctx, block);
            break;
        }
    }

    // allocate a new block
    if(!dp)
    {
        count = 1 + (inode->sectors / fs->sectors_per_block);
        status = ext2_inode_expand(ctx, dir->ino, count, true);
        if(status < 0)
        {
            return status;
        }

        block = ext2_inode_get_block(ctx, inode, count - 1);
        if(!block)
        {
            return ctx->errno;
        }

        dp = ext2_read_cached(ctx, block, true);
        if(!dp)
        {
            return ctx->errno;
        }

        dp->size = fs->block_size;
    }

    // write entry
    dp->inode = obj->ino;
    dp->length = strlen(name);
    strncpy(dp->name, name, dp->length);

    if(sb->features_required & EXT2_REQ_DIRENT_TYPE)
    {
        if(obj->flags & I_FILE)
        {
            dp->type = 1;
        }
        else if(obj->flags & I_DIR)
        {
            dp->type = 2;
        }
        else if(obj->flags & I_SYMLINK)
        {
            dp->type = 7;
        }
    }

    // adjust parent inode
    ext2_inode_settime(inode, EXT2_MTIME);
    if(obj->flags & I_DIR)
    {
        inode->links++;
    }
    ext2_inode_getattr(fs, dir, inode, 0);

    return 0;
}

static int ext2_dirent_relink(ext2_ctx_t *ctx, inode_t *dir, const char *name, inode_t *obj)
{
    ext2_dentry_t *dp;
    ext2_inode_t *inode;
    uint32_t blk;
    char filename[256];
    int status;

    // initialize iterator
    status = ext2_dirent_iter(ctx, dir->ino, true);
    if(status < 0)
    {
        return status;
    }

    // search for entry
    while(dp = ext2_dirent_next(ctx, &blk, filename), dp)
    {
        if(strcmp(name, filename) == 0)
        {
            break;
        }
    }

    if(!dp)
    {
        return -ENOENT;
    }

    // update dentry
    dp->inode = obj->ino;

    // mark as dirty
    ext2_write_cached(ctx, blk);

    // adjust parent inode
    inode = ext2_inode_read(ctx, dir->ino, true);
    ext2_inode_settime(inode, EXT2_MTIME);

    return 0;
}

static int ext2_dirent_check_empty(ext2_ctx_t *ctx, uint32_t ino)
{
    ext2_dentry_t *dp;
    ext2_inode_t *ip;
    int status;

    ip = ext2_inode_read(ctx, ino, false);
    if(!ip)
    {
        return ctx->errno;
    }

    // A directory without any subdirectories has exactly two hard links.
    // We can use this as a quick check to eliminate directories with subdirectories.
    if(ip->links > 2)
    {
        return -ENOTEMPTY;
    }

    status = ext2_dirent_iter(ctx, ino, true);
    if(status < 0)
    {
        return status;
    }

    while(dp = ext2_dirent_next(ctx, 0, 0), dp)
    {
        if(dp->inode)
        {
            return -ENOTEMPTY;
        }
    }

    return ctx->errno;
}

//
// Reading and writing of files
//

static int ext2_read(ext2_ctx_t *ctx, file_t *file, size_t size, void *buf)
{
    size_t fsize, fstart, esize;
    size_t offset, whole;
    ext2_inode_t *inode;
    inode_t *ip;
    ext2_t *fs;
    uint32_t block;
    void *blkbuf;
    int status;

    fs = ctx->fs;
    ip = file->inode;

    // (offset, whole) are in units of blocks
    // (fsize, fstart, esize) are in units of bytes

    if((file->seek + size) > ip->size) // move this to vfs?
    {
        size = ip->size - file->seek;
    }

    if(!size)
    {
        return 0;
    }

    inode = ext2_inode_read(ctx, ip->ino, false);
    if(!inode)
    {
        return ctx->errno;
    }

    offset = (file->seek / fs->block_size);
    fstart = (file->seek % fs->block_size);
    fsize  = 0;

    if(fstart)
    {
        fsize = (fs->block_size - fstart);
        if(size < fsize)
        {
            fsize = size;
        }
    }

    whole = (size - fsize) / fs->block_size;
    esize = (size - fsize) % fs->block_size;
    blkbuf = ctx->blkbuf;

    if(fsize)
    {
        block = ext2_inode_get_block(ctx, inode, offset);
        if(!block)
        {
            return ctx->errno;
        }

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
        block = ext2_inode_get_block(ctx, inode, offset);
        if(!block)
        {
            return ctx->errno;
        }

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
        block = ext2_inode_get_block(ctx, inode, offset);
        if(!block)
        {
            return ctx->errno;
        }

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

static int ext2_write(ext2_ctx_t *ctx, file_t *file, size_t size, void *buf)
{
    ext2_inode_t *inode;
    ext2_ibd_t a;
    ext2_t *fs;
    inode_t *ip;
    size_t isize, nsize, nblks;
    void *blkbuf;
    int status;
    uint32_t block;

    fs = ctx->fs;
    ip = file->inode;

    inode = ext2_inode_read(ctx, ip->ino, true);
    if(!inode)
    {
        return ctx->errno;
    }

    // check if we need to allocate more blocks
    nblks = inode->sectors / fs->sectors_per_block;
    ext2_blocks_dist(fs, &a, nblks, true);
    isize = a.dnum * fs->block_size;
    nsize = file->seek + size;

    if(nsize > isize)
    {
        nblks = (nsize + fs->block_size - 1) / fs->block_size;
        status = ext2_inode_expand(ctx, ip->ino, nblks, false);
        if(status < 0)
        {
            return status;
        }
    }

    size_t fsize, fstart, esize;
    size_t offset, whole;

    offset = (file->seek / fs->block_size);
    fstart = (file->seek % fs->block_size);
    fsize  = 0;

    if(fstart)
    {
        fsize = (fs->block_size - fstart);
        if(size < fsize)
        {
            fsize = size;
        }
    }

    whole = (size - fsize) / fs->block_size;
    esize = (size - fsize) % fs->block_size;
    blkbuf = ctx->blkbuf;

    if(fsize)
    {
        block = ext2_inode_get_block(ctx, inode, offset);
        if(!block)
        {
            return ctx->errno;
        }

        status = ext2_read_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }

        memcpy(blkbuf + fstart, buf, fsize);
        buf += fsize;
        offset++;

        status = ext2_write_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }
    }

    while(whole--)
    {
        block = ext2_inode_get_block(ctx, inode, offset);
        if(!block)
        {
            return ctx->errno;
        }

        memcpy(blkbuf, buf, fs->block_size);
        buf += fs->block_size;
        offset++;

        status = ext2_write_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }
    }

    if(esize)
    {
        block = ext2_inode_get_block(ctx, inode, offset);
        if(!block)
        {
            return ctx->errno;
        }

        if(nsize - esize < inode->size)
        {
            status = ext2_read_direct(fs, block, blkbuf);
            if(status < 0)
            {
                return status;
            }
        }

        memcpy(blkbuf, buf, esize);

        status = ext2_write_direct(fs, block, blkbuf);
        if(status < 0)
        {
            return status;
        }
    }

    if(nsize > inode->size)
    {
        inode->size = nsize;
    }

    ext2_inode_settime(inode, EXT2_MTIME);
    ext2_inode_getattr(fs, ip, inode, 0);
    file->seek += size; // move to vfs?

    return size;
}

//
// Creation and removal of files and directories
//

static int ext2_remove(ext2_ctx_t *ctx, inode_t *dir, dentry_t *dent)
{
    ext2_inode_t *inode;
    inode_t *obj;
    int status;
    int flags;

    obj   = dent->inode;
    flags = obj->flags;

    // dir: check that directory is empty
    if(flags & I_DIR)
    {
        status = ext2_dirent_check_empty(ctx, obj->ino);
        if(status < 0)
        {
            return status;
        }
    }

    // all: remove directory entry
    status = ext2_dirent_unlink(ctx, dir, dent->name, dent->inode);
    if(status < 0)
    {
        return status;
    }

    // file: check for multiple hard links
    if((flags & I_DIR) == 0)
    {
        inode = ext2_inode_read(ctx, obj->ino, true);
        if(!inode)
        {
            return ctx->errno;
        }

        if(inode->links > 1)
        {
            inode->links--;
            return 0;
        }
    }

    // all: truncate and free inode
    status = ext2_inode_truncate(ctx, obj->ino);
    if(status < 0)
    {
        return status;
    }

    status = ext2_inode_free(ctx, obj->ino);
    if(status < 0)
    {
        return status;
    }

    return 0;
}

static int ext2_mkdir(ext2_ctx_t *ctx, inode_t *dir, dentry_t *dent)
{
    ext2_dentry_t *dp;
    ext2_inode_t *inode;
    ext2_sb_t *sb;
    inode_t *obj;
    uint32_t bg;
    int status, size;

    sb = ctx->fs->sb;
    obj = dent->inode;
    bg  = (dir->ino - 1) / ctx->fs->sb->inodes_per_group;

    // find most empty block group for a new root directory
    if(dir->ino == 2)
    {
        status = ext2_bgd_search(ctx, &bg);
        if(status < 0)
        {
            return status;
        }
    }

    // allocate new inode
    inode = ext2_inode_alloc(ctx, bg, 2, obj);
    if(!inode)
    {
        return ctx->errno;
    }

    // link entry in directory
    status = ext2_dirent_link(ctx, dir, dent->name, dent->inode);
    if(status < 0)
    {
        return status;
    }

    // allocate a single block
    status = ext2_inode_expand(ctx, obj->ino, 1, true); // allocate more according to sb? check feature flag
    if(status < 0)
    {
        return status;
    }

    dp = ext2_init_cached(ctx, inode->block[0]);
    if(!dp)
    {
        return ctx->errno;
    }

    // create . and .. entries
    dp->inode = obj->ino;
    dp->length = 1;
    dp->size = sizeof(ext2_dentry_t) + dp->length;
    dp->size = align_size(dp->size , 4);
    dp->name[0] = '.';

    if(sb->features_required & EXT2_REQ_DIRENT_TYPE)
    {
        dp->type = 2;
    }

    size = ctx->fs->block_size - dp->size;
    dp = (void*)dp + dp->size;

    dp->inode = dir->ino;
    dp->length = 2;
    dp->size = size;
    dp->name[0] = '.';
    dp->name[1] = '.';

    if(sb->features_required & EXT2_REQ_DIRENT_TYPE)
    {
        dp->type = 2;
    }

    // synchronize inode data
    ext2_inode_getattr(ctx->fs, obj, inode, 0);

    return 0;
}

static int ext2_create(ext2_ctx_t *ctx, inode_t *dir, dentry_t *dent)
{
    ext2_inode_t *inode;
    inode_t *obj;
    uint32_t bg;
    int status;

    obj = dent->inode;
    bg  = (dir->ino - 1) / ctx->fs->sb->inodes_per_group;

    // allocate new inode
    inode = ext2_inode_alloc(ctx, bg, 1, obj);
    if(!inode)
    {
        return ctx->errno;
    }

    // link entry in directory
    status = ext2_dirent_link(ctx, dir, dent->name, dent->inode);
    if(status < 0)
    {
        return status;
    }

    // synchronize inode data
    ext2_inode_getattr(ctx->fs, obj, inode, 0);

    return 0;
}

static int ext2_rename(ext2_ctx_t *ctx, dentry_t *src, dentry_t *dst)
{
    ext2_dentry_t *dp;
    ext2_inode_t *inode;
    int status;

    // link or update destination
    if(dst->inode->ino)
    {
        // when the destination is a directory, it must be empty
        if(dst->inode->flags & I_DIR)
        {
            status = ext2_dirent_check_empty(ctx, dst->inode->ino);
            if(status < 0)
            {
                return status;
            }
        }

        // truncate and free
        status = ext2_inode_truncate(ctx, dst->inode->ino);
        if(status < 0)
        {
            return status;
        }

        status = ext2_inode_free(ctx, dst->inode->ino);
        if(status < 0)
        {
            return status;
        }

        // update destination
        status = ext2_dirent_relink(ctx, dst->parent->inode, dst->name, src->inode);
        if(status < 0)
        {
            return status;
        }
    }
    else
    {
        status = ext2_dirent_link(ctx, dst->parent->inode, dst->name, src->inode);
        if(status < 0)
        {
            kp_info("ext2", "link failed for: %s", dst->name);
            return status;
        }
    }

    // unlink source
    status = ext2_dirent_unlink(ctx, src->parent->inode, src->name, src->inode);
    if(status < 0)
    {
        kp_info("ext2", "unlink failed for: %s", src->name);
        return status;
    }

    // update inode
    inode = ext2_inode_read(ctx, src->inode->ino, true);
    if(!inode)
    {
        return ctx->errno;
    }

    ext2_inode_settime(inode, EXT2_CTIME);
    ext2_inode_getattr(ctx->fs, dst->inode, inode, src->inode->ino);

    // update .. for directories
    if(src->inode->flags & I_DIR)
    {
        dp = ext2_read_cached(ctx, inode->block[0], true);
        if(!dp)
        {
            return ctx->errno;
        }

        dp = (void*)dp + dp->size;
        dp->inode = dst->parent->inode->ino;
    }

    return 0;
}

//
// Wrapper functions for VFS operations
//

static int ext2fs_read(file_t *file, size_t size, void *buf)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(file->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_read(ctx, file, size, buf);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_write(file_t *file, size_t size, void *buf)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(file->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_write(ctx, file, size, buf);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_seek(file_t *file, ssize_t offset, int origin)
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

static int ext2fs_readdir(file_t *file, size_t seek, void *data)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(file->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_dirent_walk(ctx, file->inode->ino, seek, data, 0, 0);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_lookup(inode_t *dir, const char *name, inode_t *ip)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(dir->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    ip->flags = 0;
    status = ext2_dirent_walk(ctx, dir->ino, 0, 0, name, ip);
    ext2_ctx_free(ctx, status);

    if(status < 0)
    {
        return status;
    }

    if(ip->flags == 0)
    {
        return -ENOENT;
    }

    return 0;
}

static int ext2fs_truncate(inode_t *ip)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(ip->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_inode_truncate(ctx, ip->ino);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_setattr(inode_t *ip)
{
    ext2_inode_t *inode;
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(ip->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    inode = ext2_inode_read(ctx, ip->ino, true);
    if(inode)
    {
        status = 0;
        ext2_inode_setattr(inode, ip);
    }
    else
    {
        status = ctx->errno;
    }

    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_getattr(inode_t *ip)
{
    ext2_inode_t *inode;
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(ip->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    inode = ext2_inode_read(ctx, ip->ino, false);
    if(inode)
    {
        status = 0;
        ext2_inode_getattr(ctx->fs, ip, inode, 0);
    }
    else
    {
        status = ctx->errno;
    }

    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_create(dentry_t *dp)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(dp->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_create(ctx, dp->parent->inode, dp);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_remove(dentry_t *dp)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(dp->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_remove(ctx, dp->parent->inode, dp);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_rename(dentry_t *src, dentry_t *dst)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(dst->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_rename(ctx, src, dst);
    ext2_ctx_free(ctx, status);

    return status;
}

static int ext2fs_mkdir(dentry_t *dp)
{
    ext2_ctx_t *ctx;
    int status;

    ctx = ext2_ctx_alloc(dp->inode->data);
    if(!ctx)
    {
        return -ENOMEM;
    }

    status = ext2_mkdir(ctx, dp->parent->inode, dp);
    ext2_ctx_free(ctx, status);

    return status;
}

static void *ext2fs_mount(devfs_t *dev, inode_t *inode) // make a helper function for this as well?
{
    int version, status;
    ext2_inode_t *root;
    ext2_sb_t *sb;
    ext2_t *fs;

    status = blkdev_open(dev);
    if(status < 0)
    {
        kp_info("ext2", "mount: failed to open block device");
        return 0;
    }

    fs = kzalloc(sizeof(ext2_t) + 1024);
    if(!fs)
    {
        kp_info("ext2", "mount: failed to allocate memory");
        kfree(fs);
        blkdev_close(dev);
        return 0;
    }
    sb = (void*)(fs+1);

    status = blkdev_read(dev, 2, 2, sb);
    if(status < 0)
    {
        kp_info("ext2", "mount: failed to read superblock (status %d)", status);
        kfree(fs);
        blkdev_close(dev);
        return 0;
    }

    if(sb->signature != 0xEF53)
    {
        kp_info("ext2", "mount: invalid signature");
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
        kp_info("ext2", "mount: unsupported ext%d filesystem", version);
        kfree(fs);
        blkdev_close(dev);
        return 0;
    }

    fs->version = version;
    fs->block_size = (1024 << sb->log_block_size);
    fs->inodes_per_block = (fs->block_size / sb->inode_size);
    fs->sectors_per_block = (fs->block_size / 512);
    fs->bgds_total = (sb->total_blocks + sb->blocks_per_group - 1) / sb->blocks_per_group;
    fs->bgds_per_block = (fs->block_size / sizeof(ext2_bgd_t));
    fs->bgds_start = 1 + (sb->log_block_size == 0);
    fs->sb = sb;
    fs->dev = dev;
    fs->ctx = ext2_ctx_alloc(fs);

    root = ext2_inode_read(fs->ctx, 2, false);
    ext2_inode_getattr(fs, inode, root, 2);

    kp_info("ext2", "version: ext%d", version);
    kp_info("ext2", "required: %#04x", sb->features_required);
    kp_info("ext2", "readonly: %#04x", sb->features_readonly);
    kp_info("ext2", "optional: %#04x", sb->features_optional);
    kp_info("ext2", "blocks: %d (%d bytes)", sb->total_blocks, fs->block_size);
    kp_info("ext2", "inodes: %d (%d bytes)", sb->total_inodes, sb->inode_size);

    return fs;
}

static int ext2fs_umount(void *data)
{
    ext2_t *fs = data;
    int status;

    status = blkdev_write(fs->dev, 2, 2, fs->sb);
    if(status < 0)
    {
        kp_error("ext2", "failed to write superblock: %s", status);
    }

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
        .read = ext2fs_read,
        .write = ext2fs_write,
        .seek = ext2fs_seek,
        .ioctl = 0,
        .readdir = ext2fs_readdir,
        .lookup = ext2fs_lookup,
        .truncate = ext2fs_truncate,
        .setattr = ext2fs_setattr,
        .getattr = ext2fs_getattr,
        .create = ext2fs_create,
        .remove = ext2fs_remove,
        .rename = ext2fs_rename,
        .mkdir = ext2fs_mkdir,
        .rmdir = ext2fs_remove,
        .mount = ext2fs_mount,
        .umount = ext2fs_umount
    };
    vfs_register("ext2", &ops);
}
