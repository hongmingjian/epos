#include <stdarg.h>
#include <sys/types.h>
#include "kernel.h"

int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
int printk(const char *fmt,...)
{
    char buf[128];
    va_list args;
    int i, j;

    va_start(args, fmt);
    i=vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

    for(j = 0; j < i; j++)
        sys_putchar(buf[j]);

    return i;
}

