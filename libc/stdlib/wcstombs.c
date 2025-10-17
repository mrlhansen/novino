#include <stdlib.h>
#include <limits.h>

size_t wcstombs(char *str, const wchar_t *wcs, size_t n)
{
    char mbc[MB_LEN_MAX];
    size_t sz = 0;
    int status;

    while(sz < n && *wcs)
    {
        status = wctomb(mbc, *wcs);
        if(status < 0)
        {
            return -1;
        }

        if(sz + status > n)
        {
            break;
        }

        for(int i = 0; i < status; i++)
        {
            str[sz] = mbc[i];
            sz++;
        }

        wcs++;
    }

    if(sz < n)
    {
        str[sz] = '\0';
    }

    return sz;
}
