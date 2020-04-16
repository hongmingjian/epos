#include <syscall.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <unistd.h>

int printf(const char *fmt,...)
{
    char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);
    i=vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

	return write(STDOUT_FILENO, buf, i);
}

int fprintf(FILE *fp, const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);
    i=vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

	return write(STDOUT_FILENO, buf, i);
}

int vprintf(const char *fmt, va_list args)
{
    char buf[1024];
    int i;

    i=vsnprintf(buf,sizeof(buf), fmt, args);

	return write(STDOUT_FILENO, buf, i);
}

int vfprintf(FILE *fp, const char *fmt, va_list args)
{
    return vprintf(fmt, args);
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    return vsnprintf(buf, INT_MAX, fmt, args);
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(buf, INT_MAX, fmt, args);
    va_end(args);

    return i;
}

int fputc(int c, FILE *stream)
{
	char buf[1] = {c};
	if(1== write(STDOUT_FILENO, buf, 1))
		return buf[0];
	return EOF;
}