#include <stdlib.h>
#include <errno.h>

int wctomb(char *s, wchar_t wc)
{
    // 1-byte sequence
    // U+0000 to U+007F
    if(wc < 0x80)
    {
        s[0] = wc;
        return 1;
    }

    // 2-byte sequence
    // U+0080 to U+07FF
    if(wc < 0x800)
    {
        s[0] = 0xC0 | (wc >> 6);
        s[1] = 0x80 | (wc & 0x3F);
        return 2;
    }

    // surrogates
    // U+D800 to U+DFFF
    if(wc >= 0xD800 && wc <= 0xDFFF)
    {
        errno = EILSEQ;
        return -1;
    }

    // 3-byte sequence
    // U+0800 to U+FFFF
    if(wc < 0x10000)
    {
        s[0] = 0xE0 | (wc >> 12);
        s[1] = 0x80 | ((wc >> 6) & 0x3F);
        s[2] = 0x80 | (wc & 0x3F);
        return 3;
    }

    // 4-byte sequence
    // U+010000 to U+10FFFF
    if(wc < 0x110000)
    {
        s[0] = 0xF0 | (wc >> 18);
        s[1] = 0x80 | ((wc >> 12) & 0x3F);
        s[2] = 0x80 | ((wc >> 6) & 0x3F);
        s[3] = 0x80 | (wc & 0x3F);
        return 4;
    }

    errno = EILSEQ;
    return -1;
}
