#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <sys/types.h>

#define EOF (-1)

typedef struct {
    int fd;
} FILE;

#define stdin   ((FILE *)0)
#define stdout  ((FILE *)1)
#define stderr  ((FILE *)2)

int sprintf(char *buf, const char *fmt, ...);
int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list args);
int printf(const char *fmt,...);
int fprintf(FILE *fp, const char *fmt, ...);
int vfprintf(FILE *fp, const char *fmt, va_list args);
int vprintf(const char *fmt, va_list args);
int fputc(int c, FILE *fp);

#endif /* _STDIO_H */
