#include <stdlib.h>

void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*cmp)(const void*, const void*))
{
    size_t min, max, mid;
    char *bp, *cp;
    int cval;

    bp = (char*)base;
    max = num;
    min = 0;

    while(min < max)
    {
        mid = (min + max) / 2;
        cp = bp + (size * mid);
        cval = cmp(key, cp);

        if(cval == 0)
        {
            return cp;
        }
        else if(cval < 0)
        {
            max = mid;
        }
        else
        {
            min = (mid + 1);
        }
    }

    return 0;
}
