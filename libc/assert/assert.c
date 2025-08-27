#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void __assert(const char *file, unsigned int line, const char *desc)
{
    printf("Assertion failed: %s at %s, line %d", desc, file, line);
    exit(-1); // should call abort
}
