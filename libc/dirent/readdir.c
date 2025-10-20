#include <kernel/vfs/types.h>
#include <novino/syscalls.h>
#include <_stdio.h>
#include <dirent.h>
#include <string.h>

static struct dirent us_dirent;

struct dirent *readdir(DIR *dp)
{
    dirent_t *ks_dirent;
    long status;

    if(dp->flags & F_EOF)
    {
        return NULL;
    }

    // read entries
    if(dp->b.pos == dp->b.end)
    {
        status = sys_readdir(dp->fd, dp->b.size, dp->b.ptr);
        if(status < 0)
        {
            errno = -status;
            return NULL;
        }

        dp->b.pos = dp->b.ptr;
        dp->b.end = dp->b.ptr + status;

        if(status == 0)
        {
            dp->flags |= F_EOF;
            return NULL;
        }
    }

    // next entry
    ks_dirent = (void*)dp->b.pos;
    dp->b.pos += ks_dirent->length;

    // copy data
    strcpy(us_dirent.d_name, ks_dirent->name);
    us_dirent.d_type = DT_UNKNOWN;
    us_dirent.d_ino = 0;

    if(ks_dirent->flags & I_FILE)
    {
        us_dirent.d_type = DT_REG;
    }
    else if(ks_dirent->flags & I_DIR)
    {
        us_dirent.d_type = DT_DIR;
    }
    else if(ks_dirent->flags & I_BLOCK)
    {
        us_dirent.d_type = DT_BLK;
    }
    else if(ks_dirent->flags & I_STREAM)
    {
        us_dirent.d_type = DT_CHR;
    }

    return &us_dirent;
}
