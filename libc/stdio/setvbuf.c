#include <_stdio.h>

int setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
    if(fp->flags & F_TEXT)
    {
        // stdin and stdout should be line buffered
        return -1;
    }

    if(mode == _IONBF)
    {
        fp->b.size = 0;
        fp->b.mode = mode;
    }
    else if(mode == _IOLBF || mode == _IOFBF)
    {
        fp->b.mode = mode;
        if(buf)
        {
            fp->b.ptr = buf;
            fp->b.size = size;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}
