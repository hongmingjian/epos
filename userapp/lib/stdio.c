#include <syscall.h>
#include <stdio.h>
#include <stdarg.h>

int printf(const char *fmt,...)
{
    char buf[1024];
    va_list args;
    int i, j;

    va_start(args, fmt);
    i=vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

    for(j = 0; j < i; j++)
        putchar(buf[j]);

    return i;
}

int fprintf(FILE *fp, const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    int i, j;

    va_start(args, fmt);
    i=vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

    for(j = 0; j < i; j++)
        putchar(buf[j]);

    return i;
}

int vprintf(const char *fmt, va_list args)
{
    char buf[1024];
    int i, j;

    i=vsnprintf(buf,sizeof(buf), fmt, args);

    for(j = 0; j < i; j++)
        putchar(buf[j]);

    return i;
}

int vfprintf(FILE *fp, const char *fmt, va_list args)
{
    return vprintf(fmt, args);
}

int fputc(int c, FILE *stream)
{
    putchar(c);
    return c;
}