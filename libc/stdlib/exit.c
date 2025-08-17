#include <_syscall.h>
#include <stdlib.h>
#include <stdio.h>

//  All C streams (open with functions in <cstdio>) are closed (and flushed, if buffered), and all files created with tmpfile are removed.

#define SLOTS_INCREMENT 20

static int slots = 0;
static int index = 0;

static struct {
    void (*func)();
} *list = 0;

int atexit(void (*func)())
{
    void *ptr;

    // allocate memory
    if(slots == index)
    {
        slots = slots + SLOTS_INCREMENT;
        ptr = realloc(list, sizeof(*list) * slots);
        if(ptr == NULL)
        {
            slots = slots - SLOTS_INCREMENT;
            return -1;
        }
        list = ptr;
    }

    // append to list
    list[index].func = func;
    index++;
    return 0;
}

void exit(int status)
{
    // call atexit functions
    for(int i = index - 1; i >= 0; i--)
    {
        list[i].func();
    }

    // exit
    sys_exit(status);
}
