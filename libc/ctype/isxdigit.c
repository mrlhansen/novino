#include <ctype.h>

int isxdigit(int c)
{
    return ((c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x46) || (c >= 0x61 && c <= 0x66));
}
