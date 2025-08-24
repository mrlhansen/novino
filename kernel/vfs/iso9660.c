#include <kernel/vfs/iso9660.h>
#include <kernel/vfs/vfs.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>
#include <ctype.h>

static void iso9660_dirent(iso9660_dirent_t *dirent, char *filename, inode_t *inode, int joliet)
{
    iso9660_susp_t *ent;
    int susp, size;

    size = dirent->filename_length;
    filename[size] = '\0';

    if(joliet)
    {
        for(int i = 0; i < size; i++)
        {
            filename[i] = dirent->filename[2*i];
        }
    }
    else
    {
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

    inode->size = (dirent->extent_size & 0xFFFFFFFF);
    inode->ino = (dirent->extent & 0xFFFFFFFF);
    inode->blocks = (inode->size + 2047) / 2048;

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

            inode->uid = (ent->px.uid & 0xFFFFFFFF);
            inode->gid = (ent->px.gid & 0xFFFFFFFF);
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
    }

}

static int iso9660_walk_directory(inode_t *ip, size_t seek, void *data, const char *lookup_name, inode_t *lookup_inode)
{
    iso9660_dirent_t *dirent;
    iso9660_t *fs;
    inode_t inode;
    int offset = 0;
    char filename[256];
    void *buf;
    int status;
    int ns, cs;

    fs = ip->data;
    ns = ip->blocks;
    cs = 0;

    buf = kmalloc(ip->size); // do not allocate

    status = fs->dev->blk->read(fs->dev->data, ip->ino, ip->blocks, buf); // make separate read function
    if(status < 0)
    {
        return status;
    }

    while(cs < ns)
    {
        dirent = (void*)(buf + offset);
        offset += dirent->length;

        if(dirent->length == 0)
        {
            offset = 2048 * cs++;
            continue;
        }

        if(dirent->filename_length == 1)
        {
            if(dirent->filename[0] == 0)
            {
                // ignore .
                continue;
            }
            else if(dirent->filename[0] == 1)
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

    kfree(buf);

    return 0;
}

static int iso9660_read(file_t *file, size_t size, void *buf)
{
    return -1;
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
    iso9660_dirent_t *root;
    iso9660_pvd_t *pvd;
    iso9660_t *fs;
    int status;

    kp_info("iso", "trying to mount iso9660");

    pvd = kzalloc(8192); // should not be allocated?
    status = dev->blk->read(dev->data, 16, 4, pvd);
    if(status < 0)
    {
        return 0;
    }

    if(strncmp("CD001", pvd->id_standard, 5) != 0)
    {
        return 0;
    }

    fs = kzalloc(sizeof(*fs));
    if(fs == 0)
    {
        return 0;
    }

    fs->joliet_level = 0;
    fs->dev = dev;

    for(int i = 0; i < 4; i++)
    {
        root = (void*)pvd->root_record;

        if(pvd->type == ISO9660_PVD)
        {
            fs->bps = (pvd->logical_block_size & 0xFFFF);
            fs->sectors = (pvd->space_size & 0xFFFFFFFF);
            fs->root_location = (root->extent & 0xFFFFFFFF);
            fs->root_length = (root->extent_size & 0xFFFFFFFF);
        }
        else if(pvd->type == ISO9660_SVD)
        {
            if(pvd->escape_sequence[0] == 0x25 && pvd->escape_sequence[1] == 0x2F)
            {
                switch(pvd->escape_sequence[2])
                {
                case 0x40:
                    fs->joliet_level = 1;
                    break;
                case 0x43:
                    fs->joliet_level = 2;
                    break;
                case 0x45:
                    fs->joliet_level = 3;
                    break;
                default:
                    continue;
                    break;
                }
                fs->root_location = (root->extent & 0xFFFFFFFF);
                fs->root_length = (root->extent_size & 0xFFFFFFFF);
            }
        }
        else if(pvd->type == ISO9660_VTD)
        {
            break;
        }

        pvd++;
    }

    inode->ino = fs->root_location;
    inode->flags = I_DIR;
    inode->gid = 0;
    inode->uid = 0;
    inode->mode = 0555;
    inode->size = fs->root_length;
    inode->blocks = fs->root_length / fs->bps;
    inode->blksz = fs->bps;

    kp_info("iso9660", "joilet level: %d", fs->joliet_level);

    return fs;
}

int iso9660_umount(void *data)
{
    kfree(data);
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
