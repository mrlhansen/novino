#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
void __assert(const char *, unsigned int, const char *);
#define assert(expr) ((expr) ? (void)0 : __assert(__FILE__, __LINE__, #expr))
#endif

#endif
