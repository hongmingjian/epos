#include <stddef.h>
#include "kernel.h"

struct sem *sys_sem_create(int value)
{
	struct sem *sem = (struct sem *)kmalloc(sizeof(struct sem));
	sem->value = value;
	sem->wq = NULL;
	return sem;
}

void sys_sem_destory(struct sem *sem)
{
	uint32_t flags;
	save_flags_cli(flags);
	wake_up(&sem->wq, -1);
	restore_flags(flags);
	kfree(sem);
}

void sys_sem_wait(struct sem *sem)
{
	uint32_t flags;
	save_flags_cli(flags);
	sem->value--;
	if(sem->value < 0)
		sleep_on(&sem->wq);
	restore_flags(flags);
}

void sys_sem_signal(struct sem *sem)
{
	uint32_t flags;
	save_flags_cli(flags);
	sem->value++;
	if(sem->value <= 0)
		wake_up(&sem->wq, 1);
	restore_flags(flags);
}
