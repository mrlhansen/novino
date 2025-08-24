#include <kernel/vfs/iso9660.h>
#include <kernel/vfs/vfs.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/debug.h>
#include <string.h>
#include <ctype.h>

static int iso9660_readdir(file_t *file, size_t seek, void *data)
{
    iso9660_dirent_t *dirent;
    iso9660_t *fs;
    inode_t *inode;
    int offset = 0;
    char filename[256];
    void *buf;
    int status;

    inode = file->inode;
    fs = inode->data;
    int ns = inode->blocks;
    int cs = 0;

    buf = kmalloc(inode->size); // do not allocate

    status = fs->dev->blk->read(fs->dev->data, inode->ino, inode->blocks, buf); // make separate read function
    if(status < 0)
    {
        return 0;
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

        if(fs->joliet_level)
        {
            for(int i = 0; i < dirent->filename_length; i++)
            {
                filename[i] = dirent->filename[2*i];
                filename[i+1] = '\0';
            }
        }
        else
        {
            for(int i = 0; i < dirent->filename_length; i++)
            {
                if(dirent->filename[i] == ';')
                {
                    if(dirent->filename[i-1] == '.')
                    {
                        filename[i-1] = '\0';
                    }
                    break;
                }
                filename[i] = tolower(dirent->filename[i]);
                filename[i+1] = '\0';
             }
        }

        inode_t in;
        in.size = (dirent->extent_size & 0xFFFFFFFF);
        in.ino = (dirent->extent & 0xFFFFFFFF);
        in.blocks = (in.size + 2048 - 1) / 2048;
        if(dirent->flags & 0x02)
        {
            in.flags = I_DIR;
            in.mode = 0555;
        }
        else
        {
            in.flags = I_FILE;
            in.mode = 0444;
        }

        vfs_put_dirent(data, filename, &in);
    }

    return 0;
}

static int iso9660_lookup(inode_t *ip, const char *name, inode_t *inode)
{
    return -1;
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
    inode->size = fs->root_length;
    inode->blocks = (fs->root_length + fs->bps - 1) / fs->bps;
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
        .read = 0,
        .write = 0,
        .seek = 0,
        .ioctl = 0,
        .readdir = iso9660_readdir,
        .lookup = iso9660_lookup,
        .mount = iso9660_mount,
        .umount = iso9660_umount
    };
    vfs_register("iso9660", &ops);
}
