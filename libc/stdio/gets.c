#include <stdio.h>

char *gets(char *str)
{
    char *ret;

    ret = fgets(str, 8192, stdin);
    if(ret == NULL)
    {
        return NULL;
    }

    while(*ret)
    {
        if(*ret == '\n')
        {
            *ret = '\0';
            break;
        }
        ret++;
    }

    return str;
}
