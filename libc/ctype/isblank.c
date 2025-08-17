#include <ctype.h>

int isblank(int c)
{
    return ((c == 0x09) || (c == 0x20));
}
