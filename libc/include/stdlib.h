#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

#define RAND_MAX 2147483647
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* program termination */
int atexit(void (*func)());
void exit(int status);

/* environment */
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int putenv(char *string);
int unsetenv(const char *name);

/* memory allocation */
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);

/* random numbers */
int rand();
void srand(unsigned int seed);

/* string conversion */
double atof(const char *s);
int atoi(const char *s);
long atol(const char *s);

/* integer arithmetics */
int abs(int);
long labs(long);
div_t div(int, int);
ldiv_t ldiv(long, long);
int min(int, int);
long lmin(long, long);
int max(int, int);
long lmax(long, long);

/* sorting and searching */
void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*cmp)(const void*, const void*));

/* non-standard */
int ctoi(int ch);

/* multibyte */
int mblen(const char *s, size_t n);
int mbtowc(wchar_t *wc, const char *src, size_t n);
int wctomb(char *s, wchar_t wc);

#endif
