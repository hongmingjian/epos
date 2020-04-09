#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>

int task_exit(int code_exit);
int task_create(void *tos, void (*func)(void *pv), void *pv);
int task_getid();
void task_yield();
int task_wait(int tid, int *pcode_exit);

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int   munmap(void *addr, size_t len);

int putchar(int c);

int open(char *path, int mode);
#define O_RDONLY	0x0
#define O_WRONLY	0x1
#define O_RDWR      0x2
#define O_APPEND	0x2000

int close(int fd);
int read(int fd, uint8_t *buffer, size_t size);
int write(int fd, uint8_t *buffer, size_t size);
int seek(int fd, int offset, int whence);
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif /*_SYSCALL_H*/
