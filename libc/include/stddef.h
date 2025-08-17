#ifndef STDDEF_H
#define STDDEF_H

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned long size_t;
typedef signed long ssize_t;
typedef signed long ptrdiff_t;

#ifndef offsetof
#define offsetof(st,m) __builtin_offsetof(st,m)
#endif

#endif
