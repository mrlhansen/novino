#include <kernel/storage/blkdev.h>
#include <kernel/vfs/iso9660.h>
#include <kernel/vfs/vfs.h>
#include <kernel/mem/heap.h>
#include <kernel/cleanup.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>
#include <ctype.h>

DEFINE_AUTOFREE_TYPE(void)

static void iso9660_dirent(iso9660_dirent_t *dirent, char *filename, inode_t *inode, int joliet)
{
    iso9660_susp_t *ent;
    int susp, size;

    if(joliet)
    {
        size = dirent->filename_length / 2;
        filename[size] = '\0';

        for(int i = 0; i < size; i++)
        {
            // assume that filenames are pure ascii
            filename[i] = dirent->filename[2*i+1];
        }
    }
    else
    {
        size = dirent->filename_length;
        filename[size] = '\0';

        for(int i = 0; i < size; i++)
        {
            if(dirent->filename[i] == ';')
            {
                if(dirent->filename[i-1] == '.')
                {
                    filename[i-1] = '\0';
                }
                break;
            }
            filename[i] = dirent->filename[i];
        }
    }

    memset(inode, 0, sizeof(*inode));
    inode->size = dirent->extent_size;
    inode->ino = dirent->extent;

    if(dirent->flags & 0x02)
    {
        inode->flags = I_DIR;
        inode->mode = 0555;
    }
    else
    {
        inode->flags = I_FILE;
        inode->mode = 0444;
    }

    susp = offsetof(iso9660_dirent_t, filename) + dirent->filename_length;
    if(susp % 2)
    {
        susp++;
    }

    while(susp < dirent->length - 1)
    {
        ent = (void*)dirent + susp;
        susp += ent->length;

        if(ent->type == 0x5850) // PX
        {
            inode->mode = ent->px.mode & 0x1FF;
            inode->flags = 0;

            if(ent->px.mode & 040000)
            {
                inode->flags = I_DIR;
            }
            else if(ent->px.mode & 0100000)
            {
                inode->flags = I_FILE;
            }
            else if(ent->px.mode & 0120000)
            {
                inode->flags = I_SYMLINK;
            }

            inode->uid = ent->px.uid;
            inode->gid = ent->px.gid;
        }
        else if(ent->type == 0x4D4E) // NM
        {
            if(ent->nm.flags)
            {
                kp_info("iso", "found NM with non-zero flags: %x", ent->nm.flags);
            }
            size = ent->nm.length - sizeof(ent->nm);
            for(int i = 0; i < size; i++)
            {
                filename[i] = ent->nm.name[i];
            }
            filename[size] = '\0';
        }

        if(ent->length == 0)
        {
            break;
        }
    }

}

static int iso9660_walk_directory(inode_t *ip, size_t seek, void *data, const char *lookup_name, inode_t *lookup_inode)
{
    autofree(void) *tmpbuf = 0;
    iso9660_dirent_t *dirent;
    iso9660_t *fs;
    inode_t inode;
    char filename[256];
    int offset, status;
    int ns, cs;

    fs = ip->data;
    ns = ip->blocks;
    cs = 0;
    offset = 0;

    tmpbuf = kmalloc(ip->size);
    if(!tmpbuf)
    {
        return -ENOMEM;
    }

    status = blkdev_read(fs->dev, ip->ino, ip->blocks, tmpbuf);
    if(status < 0)
    {
        return status;
    }

    while(cs < ns)
    {
        dirent = (void*)(tmpbuf + offset);
        offset += dirent->length;

        if(dirent->length == 0)
        {
            cs++;
            offset = fs->bps * cs;
            continue;
        }

        if(dirent->filename_length == 1)
        {
            if(dirent->filename[0] == 0)
            {
                // ignore .
                continue;
            }
            if(dirent->filename[0] == 1)
            {
                // ignore ..
                continue;
            }
        }

        if(seek)
        {
            seek--;
            continue;
        }

        iso9660_dirent(dirent, filename, &inode, fs->joliet_level);
        inode.blksz = fs->bps;
        inode.blocks = (inode.size + inode.blksz - 1) / inode.blksz;

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

    return 0;
}

static int iso9660_read(file_t *file, size_t size, void *buf)
{
    autofree(void) *tmpbuf = 0;
    size_t fsize, fstart, esize;
    size_t offset, whole;
    iso9660_t *fs;
    int status;

    fs = file->inode->data;

    // (offset, whole) are in units of sectors
    // (fsize, fstart, esize) are in units of bytes

    if((file->seek + size) > file->inode->size) // move this to vfs?
    {
        size = file->inode->size - file->seek;
    }

    if(!size)
    {
        return 0;
    }

    offset = (file->seek / fs->bps) + file->inode->ino;
    fstart = (file->seek % fs->bps);

    if(fstart)
    {
        fsize = (fs->bps - fstart);
        if(size < fsize)
        {
            fsize = size;
        }
    }
    else
    {
        fsize = 0;
    }

    whole = (size - fsize) / fs->bps;
    esize = (size - fsize) % fs->bps;

    tmpbuf = kmalloc(fs->bps);
    if(!tmpbuf)
    {
        return -ENOMEM;
    }

    if(fsize)
    {
        status = blkdev_read(fs->dev, offset, 1, tmpbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, tmpbuf + fstart, fsize);
        buf += fsize;
        offset++;
    }

    if(whole)
    {
        status = blkdev_read(fs->dev, offset, whole, buf);
        if(status < 0)
        {
            return status;
        }
        buf += whole * fs->bps;
        offset += whole;
    }

    if(esize)
    {
        status = blkdev_read(fs->dev, offset, 1, tmpbuf);
        if(status < 0)
        {
            return status;
        }
        memcpy(buf, tmpbuf, esize);
    }

    file->seek += size;
    return size;
}

static int iso9660_seek(file_t *file, ssize_t offset, int origin)
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

static int iso9660_readdir(file_t *file, size_t seek, void *data)
{
    return iso9660_walk_directory(file->inode, seek, data, 0, 0);
}

static int iso9660_lookup(inode_t *ip, const char *name, inode_t *inode)
{
    int status;

    inode->flags = 0;
    status = iso9660_walk_directory(ip, 0, 0, name, inode);
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

static void *iso9660_mount(devfs_t *dev, inode_t *inode)
{
    autofree(void) *tmpbuf = 0;
    iso9660_dirent_t *root;
    iso9660_pvd_t *vds, *pvd;
    iso9660_t *fs;
    int status;

    status = blkdev_open(dev);
    if(status < 0)
    {
        return 0;
    }

    tmpbuf = kmalloc(8192);
    if(!tmpbuf)
    {
        blkdev_close(dev);
        return 0;
    }

    vds = tmpbuf;
    status = blkdev_read(dev, 16, 4, vds);
    if(status < 0)
    {
        blkdev_close(dev);
        return 0;
    }

    if(strncmp("CD001", vds->id_standard, 5) != 0)
    {
        blkdev_close(dev);
        return 0;
    }

    fs = kzalloc(sizeof(iso9660_t));
    if(fs == 0)
    {
        blkdev_close(dev);
        return 0;
    }

    fs->joliet_level = 0;
    fs->dev = dev;
    pvd = 0;

    for(int i = 0; i < 4; i++)
    {
        root = (void*)vds->root_record;

        if(vds->type == ISO9660_PVD)
        {
            fs->bps = vds->logical_block_size;
            fs->sectors = vds->space_size;
            fs->root_extent = root->extent;
            fs->root_size = root->extent_size;
            pvd = vds;
        }
        else if(vds->type == ISO9660_SVD)
        {
            if(vds->escape_sequence[0] == 0x25 && vds->escape_sequence[1] == 0x2F)
            {
                switch(vds->escape_sequence[2])
                {
                    case 0x40:
                        fs->joliet_level = 1; // Sequence: %\@
                        break;
                    case 0x43:
                        fs->joliet_level = 2; // Sequence: %\C
                        break;
                    case 0x45:
                        fs->joliet_level = 3; // Sequence: %\E
                        break;
                    default:
                        continue;
                        break;
                }
                fs->root_extent = root->extent;
                fs->root_size = root->extent_size;
            }
        }
        else if(vds->type == ISO9660_VTD)
        {
            break;
        }

        vds++;
    }

    if(!pvd)
    {
        kfree(fs);
        blkdev_close(dev);
        return 0;
    }

    inode->ino = fs->root_extent;
    inode->flags = I_DIR;
    inode->gid = 0;
    inode->uid = 0;
    inode->mode = 0555;
    inode->size = fs->root_size;
    inode->blocks = fs->root_size / fs->bps;
    inode->blksz = fs->bps;

    kp_info("isofs", "volume identifier: %.32s", pvd->id_volume);
    if(fs->joliet_level)
    {
        kp_info("isofs", "joilet level: %d", fs->joliet_level);
    }
    kp_info("isofs", "volume geometry: %d sectors, %d bps", fs->sectors, fs->bps);

    return fs;
}

int iso9660_umount(void *data)
{
    iso9660_t *fs = data;
    blkdev_close(fs->dev);
    kfree(fs);
    return 0;
}

void iso9660_init()
{
    static vfs_ops_t ops = {
        .open = 0,
        .close = 0,
        .read = iso9660_read,
        .write = 0,
        .seek = iso9660_seek,
        .ioctl = 0,
        .readdir = iso9660_readdir,
        .lookup = iso9660_lookup,
        .mount = iso9660_mount,
        .umount = iso9660_umount
    };
    vfs_register("iso9660", &ops);
}
