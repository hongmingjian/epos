#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>
#include <ioctl.h>

int task_exit(int code_exit);
int task_create(void *tos, void (*func)(void *pv), void *pv);
int task_getid();
void task_yield();
int task_wait(int tid, int *pcode_exit);

unsigned sleep(unsigned seconds);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

int putchar(int c);

#endif /*_SYSCALL_H*/
