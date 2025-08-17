#include <_syscall.h>
#include <_stdio.h>
#include <string.h>

size_t fread(void *ptr, size_t size, size_t count, FILE *fp)
{
    size_t len, pos, rem;
    long status;
    char *buf;

    // trivial case
    if(size == 0 || count == 0)
    {
        return 0;
    }

    // stream must be open for reading
    if((fp->flags & F_READ) == 0)
    {
        // set errno
        fp->flags |= F_ERR;
        return EOF;
    }

    // current stream direction
    if(fp->mode == F_WRITE)
    {
        // do something
        return EOF;
    }
    else
    {
        fp->mode = F_READ;
    }

    // check eof flag
    if(fp->flags & F_EOF)
    {
        return EOF;
    }

    buf = ptr;
    len = size * count;
    pos = 0;

    // ungetc buffer
    while(fp->u.pos > fp->u.ptr && len > 0)
    {
        *buf = *--fp->u.pos;
        pos++;

        if(fp->b.mode == _IOLBF)
        {
            if(*buf == '\n')
            {
                len = 0;
                break;
            }
        }

        buf++;
        len--;
    }

    if(len == 0)
    {
        return pos / size;
    }

    // read depending on buffering mode
    if(fp->b.mode == _IONBF)
    {
        status = __libc_fp_read(fp, len, buf);
        if(status > 0)
        {
            pos += status;
        }
    }
    else
    {
        while(len)
        {
            if(fp->b.mode == _IOLBF) // line buffering
            {
                while(fp->b.pos < fp->b.end && len > 0)
                {
                    *buf = *fp->b.pos++;
                    pos++;

                    if(*buf == '\n')
                    {
                        len = 0;
                        break;
                    }

                    buf++;
                    len--;
                }
            }
            else // full buffering
            {
                rem = (size_t)(fp->b.end - fp->b.pos);
                if(rem > len)
                {
                    rem = len;
                }

                memcpy(buf, fp->b.pos, rem);
                fp->b.pos += rem;
                pos += rem;
                buf += rem;
                len -= rem;
            }

            // stop
            if(len == 0)
            {
                break;
            }

            // read more data
            status = __libc_fp_read(fp, fp->b.size, fp->b.ptr);
            if(status < 0)
            {
                break;
            }
            fp->b.pos = fp->b.ptr;
            fp->b.end = fp->b.ptr + status;

            // end of file
            if(fp->flags & F_EOF)
            {
                break;
            }
        }
    }

    return pos / size;
}
