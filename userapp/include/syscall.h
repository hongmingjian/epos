#ifndef _SYSCALL_H
#define _SYSCALL_H

int task_exit(int code_exit);
int task_create(void *tos, void (*func)(void *pv), void *pv);
int task_getid();
void task_yield();
int task_wait(int tid, int *pcode_exit);


#endif /*_SYSCALL_H*/
