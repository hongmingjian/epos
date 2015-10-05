/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include "../global.h"
#include "syscall.h"
#include "math.h"
#include "graphics.h"

///////////////////辅助函数///////////////////////
#include "../tlsf/tlsf.h"
extern char end[];
void *malloc(size_t bytes)
{ return malloc_ex(bytes, end); }
void *realloc(void *oldptr, size_t bytes)
{ return realloc_ex(oldptr, bytes, end); }
void free(void *ptr)
{ free_ex(ptr, end); }

#include <stdarg.h>
int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
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

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
    init_memory_pool(64*1024*1024, end);
}

#define DELAY(n) do { \
    unsigned __n=(n); \
    while(__n--); \
} while(0);
///////////////////辅助函数///////////////////////

/**
 * 这个系统第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{
    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here




    while(1)
        ;
    task_exit(0);
}

