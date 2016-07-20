/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <syscall.h>

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
}

/**
 * 第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{
    while(1)
        putchar('Z');
}

