#include <ctype.h>

int isprint(int c)
{
    return (!iscntrl(c) || isspace(c));
}
