#include <stdlib.h>
#include <errno.h>

// 1-byte : 0xxxxxxx (7 bits)
// 2-byte : 110xxxxx 10yyyyyy (5+6 bits)
// 3-byte : 1110xxxx 10yyyyyy 10zzzzzz (4+2x6 bits)
// 4-byte : 11110xxx 10yyyyyy 10zzzzzz 10vvvvvv (3+3x6 bits)

int mbtowc(wchar_t *wc, const char *str, size_t n)
{
    const unsigned char *s = (const void*)str;
    wchar_t val = *s++;
    wchar_t ch = 0;
    size_t len = 1;

    if(!s)
    {
        return 0;
    }

    if(!n)
    {
        errno = EILSEQ;
        return -1;
    }

    if(!wc)
    {
        wc = &val;
    }

    if(val < 128)
    {
        *wc = val;
        return (val > 0);
    }

    int bit_mask = 0x3F;
    int bit_shift = 0;
    int bit_total = 0;
    int t = val;

    while((t & 0xC0) == 0xC0)
    {
        if(len == n)
        {
            errno = EILSEQ;
            return -1;
        }

        val = *s++;
        len++;

        if((val & 0xC0) != 0x80)
        {
            errno = EILSEQ;
            return -1;
        }

        t = (t << 1) & 0xFF;
        bit_total += 6;
        bit_mask >>= 1;
        bit_shift++;
        ch = (ch << 6) | (val & 0x3F);
    }

    ch |= ((t >> bit_shift) & bit_mask) << bit_total;
    *wc = ch;

    return len;
}
