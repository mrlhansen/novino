#include <_stdio.h>

int vsprintf(char *str, const char *fmt, va_list args)
{
    FILE fp;
    int len;

    // fake file struct
    fp.fd = -1;
    fp.mode = 0;
    fp.flags = F_WRITE;

    fp.b.size = 8192;
    fp.b.ptr = str;
    fp.b.pos = fp.b.ptr;
    fp.b.end = fp.b.ptr + fp.b.size;
    fp.b.mode = _IOFBF;

    // call vfprintf
    len = vfprintf(&fp, fmt, args);
    str[len] = '\0';
    return len;
}
