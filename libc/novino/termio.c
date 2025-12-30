#include <kernel/term/term.h>
#include <novino/termio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

char *tiogets(char *buf, int len)
{
    len = read(STDIN_FILENO, buf, len-1);
    if(len == 0)
    {
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}

int tiogetescape(const char *str, int *key, int *va, int *vb)
{
    int csi = 0;
    int len = 0;
    int a = 0;
    int b = -1;
    int ch;

    if(*str != '\e')
    {
        return 0;
    }

    str++;
    len++;

    if(*str != '[')
    {
        *key = '\e';
        return len;
    }

    while(*str)
    {
        ch = *str++;
        len++;

        if(csi)
        {
            if(ch >= 0x40 && ch <= 0x7E)
            {
                break;
            }
            else if(ch == ';')
            {
                b = a;
                a = 0;
            }
            else
            {
                a = 10*a + ctoi(ch);
            }
        }
        else if(ch == '[')
        {
            csi = 1;
        }
        else
        {
            break;
        }
    }

    if(csi)
    {
        *key = ch;
        if(b > 0)
        {
            *va = b;
            *vb = a;
        }
        else
        {
            *va = a;
            *vb = b;
        }
    }
    else
    {
        *key = 0;
    }

    return len;
}

int tiosetflags(int flags)
{
    return ioctl(STDOUT_FILENO, TIOSETFLAGS, flags);
}

int tiogetflags(int *flags)
{
    return ioctl(STDOUT_FILENO, TIOGETFLAGS, flags);
}

int tiogetwinsz(tiowinsz_t *w)
{
    return ioctl(STDOUT_FILENO, TIOGETWINSZ, w);
}
