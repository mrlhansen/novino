#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>

static char match[256];
static char path[256];

static struct dirent **list = 0;
static int list_cap;
static int list_len;

static void list_grow()
{
    struct dirent *dent;
    int list_old;

    if(!list)
    {
        list_old = 0;
        list_cap = 32;
    }
    else
    {
        list_old = list_cap;
        list_cap = 2 * list_cap;
    }

    list = realloc(list, list_cap * sizeof(void*));
    dent = calloc(list_cap - list_old, sizeof(*dent));

    for(int i = list_old; i < list_cap; i++)
    {
        list[i] = dent++;
    }
}

static void list_add(struct dirent *dent)
{
    if(list_len == list_cap)
    {
        list_grow();
    }
    memcpy(list[list_len], dent, sizeof(*dent));
    list_len++;
}

static void list_reset()
{
    if(!list)
    {
        list_grow();
    }
    list_len = 0;
}

static void list_sort()
{
    void *tmp;
    int sorted;
    int gap;

    gap = list_len; // simple comb sort
    sorted = 0;

    while(!sorted)
    {
        gap = gap / 1.3;
        if(gap < 1)
        {
            gap = 1;
            sorted = 1;
        }
        else if(gap == 9 || gap == 10)
        {
            gap = 11;
        }

        for(int i = 0; i + gap < list_len; i++)
        {
            if(strcasecmp(list[i]->d_name, list[i+gap]->d_name) > 0)
            {
                tmp = list[i];
                list[i] = list[i+gap];
                list[i+gap] = tmp;
                sorted = 0;
            }
        }
    }
}

char *autocomplete(char *str, int suggest)
{
    struct dirent *dent;
    char *delim, *a, *b;
    DIR *dp;

    if(suggest)
    {
        list_reset();
    }

    memset(match, 0, sizeof(match));
    memset(path, 0, sizeof(path));

    if(*str == '/')
    {
        strcpy(path, "/");
    }
    else
    {
        strcpy(path, ".");
    }

    delim = strrchr(str, '/');
    if(delim)
    {
        strncpy(path, str, (size_t)(delim - str));
        str = delim + 1;
    }

    int maxlen = 0;
    int curlen = 0;
    int mcnt = 0;
    int mlen = 0;
    int len = strlen(str);

    dp = opendir(path);
    if(!dp)
    {
        return 0;
    }

    while(dent = readdir(dp), dent)
    {
        if(dent->d_name[0] == '.')
        {
            if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            {
                if(str[0] != '.')
                {
                    continue;
                }
            }
        }

        if(strncmp(str, dent->d_name, len) == 0)
        {
            mcnt++;

            if(suggest)
            {
                list_add(dent);
                curlen = strlen(dent->d_name);
                if(curlen > maxlen)
                {
                    maxlen = curlen;
                }
                continue;
            }

            if(mcnt > 1)
            {
                a = match;
                b = dent->d_name;
                mlen = 0;

                while(*a == *b && *a)
                {
                    a++;
                    b++;
                    mlen++;
                }

                match[mlen] = '\0';
                continue;
            }

            if(dent->d_type == DT_DIR)
            {
                mlen = sprintf(match, "%s/", dent->d_name);
            }
            else
            {
                mlen = sprintf(match, "%s ", dent->d_name);
            }
        }
    }

    closedir(dp);

    if(suggest)
    {
        list_sort();

        maxlen = maxlen + 2;
        curlen = max(1, 80/maxlen);

        for(int i = 0; i < list_len; i++)
        {
            if((i % curlen) == 0)
            {
                putchar('\n');
            }

            dent = list[i];
            len = 0;
            if(curlen > 1)
            {
                len = maxlen - strlen(dent->d_name);
            }

            printf("%s%-*s", dent->d_name, len, (dent->d_type == DT_DIR) ? "/" : "");
        }

        if(list_len)
        {
            putchar('\n');
            mlen = len + 1;
        }
    }

    if(mlen > len)
    {
        return match + len;
    }

    return NULL;
}
