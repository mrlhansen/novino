#include <ctype.h>

int isspace(int c)
{
    return ((c >= 0x09 && c <= 0x0D) || (c == 0x20));
}
