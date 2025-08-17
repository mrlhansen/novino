#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stddef.h>

typedef long fpos_t;
typedef struct __libc_fd FILE;

/* standard streams */
extern FILE *stdin;
extern FILE *stdout;

/* file operations */
FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *fp);
size_t fread(void *ptr, size_t size, size_t count, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *fp);

int feof(FILE *fp);
int ferror(FILE *fp);
void clearerr(FILE *fp);
int fflush(FILE *fp);

int fgetpos(FILE *fp, fpos_t *pos);
int fsetpos(FILE *fp, const fpos_t *pos);
int fseek(FILE *fp, long offset, int origin);
long ftell(FILE *fp);
void rewind(FILE *fp);

int fgetc(FILE *fp);
int fputc(int ch, FILE *fp);
int ungetc(int ch, FILE *fp);
int getchar();
int putchar(int ch);

char *fgets(char *str, int size, FILE *fp);
int fputs(const char *str, FILE *fp);
char *gets(char *str);
int puts(const char *str);

/* formatted print */
int printf(const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int fprintf(FILE *fp, const char *fmt, ...);
int vsprintf(char *str, const char *fmt, va_list args);
int vfprintf(FILE *fp, const char *fmt, va_list args);

int scanf(const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);
int fscanf(FILE *fp, const char *fmt, ...);
int vsscanf(const char *str, const char *fmt, va_list args);
int vfscanf(FILE *fp, const char *fmt, va_list args);

/* end of file */
#define EOF (-1)

/* used with fseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* buffering */
#define BUFSIZ 1024
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

void setbuf(FILE *fp, char *buf);
int setvbuf(FILE *fp, char *buf, int mode, size_t size);

#endif
