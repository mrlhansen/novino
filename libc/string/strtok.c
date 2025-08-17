#include <string.h>

char *strtok_r(char *str, char const *ct, char **sp)
{
    char *tmp = *sp;
    const char *p = ct;

    if(str == NULL)
    {
        if(tmp == NULL)
        {
            return NULL;
        }
        str = tmp;
    }
    else
    {
        tmp = str;
    }

    while(*p && *str)
    {
        if(*str == *p)
        {
            str++;
            p = ct;
            continue;
        }
        p++;
    }

    if(*str == '\0')
    {
        *sp = NULL;
        return NULL;
    }
    else
    {
        tmp = str;
    }

    while(*tmp)
    {
        p = ct;
        while(*p)
        {
            if(*tmp == *p++)
            {
                *tmp++ = '\0';
                *sp = tmp;
                return str;
            }
        }
        tmp++;
    }

    *sp = NULL;
    return str;
}

char *strtok(char *str, const char *ct)
{
    static char *sp = NULL;
    return strtok_r(str, ct, &sp);
}
