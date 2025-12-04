#include <_stdlib.h>
#include <string.h>
#include <errno.h>

static int *state = 0;  // Allocation state (1 if variable is allocated by libc, 0 otherwise)
static int cap = 0;     // Total capacity
static int cnt = 0;     // Current count

static int __libc_init_environ()
{
    char **new_environ = 0;

    // size of environment
    while(environ[cnt])
    {
        cnt++;
    }

    // allocate memory
    cap = 10 + 2 * cnt;
    new_environ = calloc(cap+1, sizeof(char*));
    if(!new_environ)
    {
        errno = ENOMEM;
        return -1;
    }

    state = calloc(cap+1, sizeof(int));
    if(!state)
    {
        errno = ENOMEM;
        return -1;
    }

    // copy variables
    for(int i = 0; i < cnt; i++)
    {
        new_environ[i] = environ[i];
        state[i] = 0;
    }

    environ = new_environ;
    return 0;
}

static int __libc_resize_environ()
{
    char **new_environ = 0;
    int *new_state = 0;

    // allocate memory
    cap = 2 * cnt;
    new_environ = calloc(cap+1, sizeof(char*));
    if(!new_environ)
    {
        errno = ENOMEM;
        return -1;
    }

    new_state = calloc(cap+1, sizeof(int));
    if(!new_state)
    {
        errno = ENOMEM;
        return -1;
    }

    // copy variables
    for(int i = 0; i < cnt; i++)
    {
        new_environ[i] = environ[i];
        new_state[i] = state[i];
    }

    free(environ);
    free(state);

    environ = new_environ;
    state = new_state;

    return 0;
}

int __libc_getenv(const char *name)
{
    char *pos, *str;
    int n = 0;
    int nlen;
    int elen;

    pos = strchr(name, '=');
    if(pos != NULL)
    {
        nlen = (int)(pos - name);
    }
    else
    {
        nlen = strlen(name);
    }

    while(str = environ[n++], str)
    {
        pos = strchr(str, '=');
        if(pos == NULL)
        {
            continue;
        }

        elen = (int)(pos - str);
        if(nlen != elen)
        {
            continue;
        }

        if(strncmp(str, name, nlen) == 0)
        {
            return n-1;
        }
    }

    return -1;
}

int __libc_putenv(const char *string, int allocated, int pos)
{
    if(!cap)
    {
        if(__libc_init_environ() < 0)
        {
            return -1;
        }
    }
    else if(cap == cnt)
    {
        if(__libc_resize_environ() < 0)
        {
            return -1;
        }
    }

    if(pos < 0)
    {
        pos = cnt;
        cnt++;
    }

    if(state[pos])
    {
        free(environ[pos]);
    }

    environ[pos] = (char*)string;
    state[pos] = allocated;

    return 0;
}

int __libc_unsetenv(const char *name)
{
    int pos;

    if(!cap)
    {
        if(__libc_init_environ() < 0)
        {
            return -1;
        }
    }

    pos = __libc_getenv(name);
    if(pos < 0)
    {
        return 0;
    }

    if(state[pos])
    {
        free(environ[pos]);
    }

    for(int i = pos; i < cnt; i++)
    {
        environ[i] = environ[i+1];
        state[i] = state[i+1];
    }

    cnt--;
    return 0;
}
