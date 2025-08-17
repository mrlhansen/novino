#include <kernel/term/console.h>
#include <kernel/x86/ioports.h>
#include <kernel/debug.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static int verbose = 0;

void kprint(loglevel_t level, const char *name, const char *file, int line, const char *fmt, ...)
{
    char sbuf[256];
    char obuf[256];
    va_list args;

    va_start(args, fmt);
    vsprintf(sbuf, fmt, args);
    va_end(args);

    if(verbose)
    {
        sprintf(obuf, "[ %-5s ] %s:%d: %s\n", name, file, line, sbuf);
    }
    else
    {
        sprintf(obuf, "[ %-5s ] %s\n", name, sbuf);
    }

    console_puts(console_default(), obuf);

    // Port E9 hack
    for(int i = 0; i < strlen(obuf); i++)
    {
        outportb(0xE9, obuf[i]);
    }
}
