#include <stdlib.h>

int ctoi(int ch)
{
    if(ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if(ch >= 'A' && ch <= 'F')
    {
        return 10 + ch - 'A';
    }
    if(ch >= 'a' && ch <= 'f')
    {
        return 10 + ch - 'a';
    }
    return -1;
}
