/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <netinet/in.h>
#include "graphics.h"

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
    void *heap_base = mmap(NULL, 64*1024*1024, 0, MAP_PRIVATE|MAP_ANON, -1, 0);
    init_memory_pool(64*1024*1024, heap_base);
}

#define DELAY(n) do { \
    unsigned __n=(n); \
    while(__n--); \
} while(0);
///////////////////辅助函数///////////////////////

/**
 * 第一个运行在用户模式的线程所执行的函数
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

