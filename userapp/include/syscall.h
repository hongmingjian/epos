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

#endif /*_SYSCALL_H*/
