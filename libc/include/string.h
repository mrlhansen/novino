#ifndef STRING_H
#define STRING_H

#include <stddef.h>

/* copying */

void *memcpy(void *dest, const void *src, size_t len);
void *memmove(void *dest, const void *src, size_t len);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t len);

/* concatenation */

char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t len);

/* comparison */

int memcmp(const void *s1, const void *s2, size_t len);
int strcmp(const char *s1, const char *s2);
int strcoll(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, int len);
size_t strxfrm(char *dest, const char *src, size_t n);

/* searching */

void *memchr(const void *ptr, int val, size_t len);
char *strchr(const char *str, int val);
size_t strcspn(const char *s1, const char *s2);
char *strpbrk(char const *cs, char const *ct);
char *strrchr(const char *str, int val);
size_t strspn(char const *s, char const *accept);
char *strstr(const char *s1, const char *s2);
char *strtok(char *str, const char *ct);
char *strtok_r(char *str, char const *ct, char **sp);

/* other */

void *memset(void *dest, int val, size_t len);
char *strerror(int errnum);
size_t strlen(const char *str);

/* non-standard */

ssize_t strscpy(char *dest, const char *src, size_t len);
char* strrev(char *str);
char *strtoupper(char *str);
char *strtolower(char *str);
char *strtrim(char *dest, const char *src);
char *strdup(const char *str);


#endif
