/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <syscall.h>
#include <stdio.h>

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
    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);
    while(1) {
        putchar('Z');
		nanosleep((const struct timespec[]){{5, 0}}, NULL);
        putchar('A');
		sleep(1);
	}
}

