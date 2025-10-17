#include <stdlib.h>
#include <string.h>

size_t mbstowcs(wchar_t *wcs, const char *str, size_t n)
{
    size_t sz = 0;
    int status;
    int len, pos;

    len = strlen(str);
    pos = 0;

    while(sz < n && *str)
    {
        status = mbtowc(wcs, str + pos, len - pos);
        if(status < 0)
        {
            return -1;
        }
        wcs++;
        sz++;
        pos += status;
    }

    if(sz < n)
    {
        *wcs = '\0';
    }

    return sz;
}
