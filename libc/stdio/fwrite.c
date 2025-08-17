#include <_syscall.h>
#include <_stdio.h>
#include <string.h>

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *fp)
{
    size_t len, pos, cap;
    long status;
    char *buf;

    // trivial case
    if(size == 0 || count == 0)
    {
        return 0;
    }

    // stream must be open for writing
    if((fp->flags & F_WRITE) == 0)
    {
        // set errno
        fp->flags |= F_ERR;
        return EOF;
    }

    // current stream direction
    if(fp->mode == F_READ) // unless EOF flag is set!
    {
        // do something
        return EOF;
    }
    else
    {
        fp->mode = F_WRITE;
    }

    buf = (void*)ptr;
    len = size * count;
    pos = 0;

    // write depending on buffering mode
    if(fp->b.mode == _IONBF)
    {
        status = __libc_fp_write(fp, len, buf);
        if(status > 0)
        {
            pos += status;
        }
    }
    else
    {
        fp->b.end = fp->b.ptr + fp->b.size;
        while(len)
        {
            if(fp->b.mode == _IOLBF) // line buffering
            {
                while(fp->b.pos < fp->b.end && len > 0)
                {
                    *fp->b.pos++ = *buf;
                    pos++;

                    if(*buf == '\n')
                    {
                        fp->b.end = fp->b.pos;
                    }

                    buf++;
                    len--;
                }
            }
            else // full buffering
            {
                cap = (size_t)(fp->b.end - fp->b.pos);
                if(cap > len)
                {
                    cap = len;
                }

                memcpy(fp->b.pos, buf, cap);
                fp->b.pos += cap;
                pos += cap;
                buf += cap;
                len -= cap;
            }

            if(fp->b.end == fp->b.pos)
            {
                status = __libc_fp_flush(fp);
                if(status < 0)
                {
                    break;
                }
            }
        }
    }

    return pos / size;
}
