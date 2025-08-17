#include <_stdio.h>
#include <string.h>

int vsscanf(const char *str, const char *fmt, va_list args)
{
    char ungetbuf[UNGETSIZ];
    FILE fp;

    // fake file struct
    fp.fd = -1;
    fp.mode = 0;
    fp.flags = F_READ;

    fp.b.size = 1 + strlen(str);
    fp.b.ptr = (char*)str;
    fp.b.pos = fp.b.ptr;
    fp.b.end = fp.b.ptr + fp.b.size;
    fp.b.mode = _IOFBF;

    fp.u.ptr = ungetbuf;
    fp.u.pos = fp.u.ptr;

    // call vfscanf
    return vfscanf(&fp, fmt, args);
}
