/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <syscall.h>

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
	open("$:/uart1", O_RDONLY);//=STDIN_FILENO
	open("$:/uart1", O_WRONLY);//=STDOUT_FILENO
	open("$:/uart1", O_WRONLY);//=STDERR_FILENO

    size_t heap_size = 32*1024*1024;
    void  *heap_base = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	g_heap = tlsf_create_with_pool(heap_base, heap_size);
}

void tsk_foo(void *pv);
void tsk_heartbeat(void *pv);

/**
 * 第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{
    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here

    char *path = "$:/sd0";

	if(0) {
		int fd = open(path, O_WRONLY);
		if(fd < 0) {
			printf("failed to open %s\r\n", path);
		} else {
			unsigned char buf[514]={0};
			int i;

			for(i = 1; i < sizeof(buf); i++) {
				//if(buf[i-1] == 0xff)
					buf[i] = 0;
				//else
					//buf[i] = buf[i-1]+1;
			}

			printf("Seek to %d\r\n", lseek(fd, 512, SEEK_SET));
			int nbyte = write(fd, buf, 1);
			if(nbyte == 1) {
			} else {
				printf("write returns %d\r\n", nbyte);
			}

			printf("Seek to %d\r\n", lseek(fd, 510, SEEK_CUR));
			nbyte = write(fd, buf, sizeof(buf));
			if(nbyte == sizeof(buf)) {
			} else {
				printf("write returns %d\r\n", nbyte);
			}
			close(fd);
		}
	}

	{
		int fd = open(path, O_RDONLY);
		if(fd < 0) {
			printf("failed to open %s\r\n", path);
		} else {
			char buf[512];
			//printf("Seek to %d\r\n", seek(fd, 0, SEEK_SET));
			int nbyte = read(fd, buf, sizeof(buf));
			if(nbyte == sizeof(buf)) {
				int i;
				for(i = 0; i < sizeof(buf); i++) {
					if(i%16 == 0)
						printf("\r\n");
					printf("%02X ", buf[i]);
				}
			} else {
				printf("read returns %d\r\n", nbyte);
			}
			close(fd);
		}
	}
	unsigned int  stack_size = 1024*1024;
	
	int tid_heartbeat;
	unsigned char *stack_heartbeat = (unsigned char *)malloc(stack_size );
	tid_heartbeat = task_create(stack_heartbeat+stack_size, &tsk_heartbeat, (void *)0);

	int tid_foo;
	unsigned char *stack_foo = (unsigned char *)malloc(stack_size );
	tid_foo = task_create(stack_foo+stack_size, &tsk_foo, (void *)5);
	task_wait(tid_foo, NULL);
	printf("task #%d exited\r\n", tid_foo);

	while(1) {
		char c;
		if(1 == read(STDIN_FILENO, &c, 1))
			write(STDOUT_FILENO, &c, 1);
	}
	
	task_wait(tid_heartbeat, NULL);
	
	while(1)
		;
	task_exit(0);
}

void tsk_heartbeat(void *pv)
{
	int fd=open("$:/led0", 0);
	int val=1;
	if(fd >= 0) {
		while(1) {
			ioctl(fd, val, NULL);
			sleep(1);
			val ^= 1;
		}
		close(fd);
	} else
		printf("Failed to open $:/led0\r\n");

	task_exit(0);
}

void tsk_foo(void *pv)
{
	int n = (int)pv;
	unsigned char *p;
    printf("This is task foo with tid=%d\r\n", task_getid());
    while(--n) {
        printf("Z");
		nanosleep((const struct timespec[]){{5, 0}}, NULL);
        printf("A");
		sleep(1);
		p = (unsigned char *)malloc(4096);
		printf("0x%08x\r\n", p);
		*p = n;
		free(p);
	}
    task_exit(0);
}
