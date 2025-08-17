#include <ctype.h>

int isalpha(int c)
{
    return ((c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A));
}
