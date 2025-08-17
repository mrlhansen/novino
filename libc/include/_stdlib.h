#pragma once

#include <stdlib.h>

extern char **environ;
int __libc_getenv(const char *name);
int __libc_putenv(const char *string, int allocated, int overwrite);
int __libc_unsetenv(const char *name);

int __libc_heap_init();
