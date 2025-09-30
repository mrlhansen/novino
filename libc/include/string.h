#ifndef STRING_H
#define STRING_H

#include <stddef.h>

/* copying */
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);

/* concatenation */
char *strcat(char *, const char *);
char *strncat(char *, const char *, size_t);

/* comparison */
int memcmp(const void *, const void *, size_t);
int strcmp(const char *, const char *);
int strcoll(const char *, const char *);
int strncmp(const char *, const char *, size_t);
size_t strxfrm(char *, const char *, size_t);

/* searching */
void *memchr(const void *, int, size_t);
char *strchr(const char *, int);
size_t strcspn(const char *, const char *);
char *strpbrk(const char *, const char *);
char *strrchr(const char *, int);
size_t strspn(const char *, const char *);
char *strstr(const char *, const char *);
char *strtok(char *, const char *);
char *strtok_r(char *, const char *, char **);

/* other */
void *memset(void *, int, size_t);
char *strerror(int);
size_t strlen(const char *);

/* non-standard */
ssize_t strscpy(char*, const char*, size_t);
char *strrev(char *);
char *strtoupper(char *);
char *strtolower(char *);
char *strtrim(char *, const char *);

#endif
