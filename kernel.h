/**
 *
 * Copyright (C) 2013 Hong MingJian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#ifndef _KERNEL_H
#define _KERNEL_H

#include "machdep.h"

extern void (*intr_vector[])(uint32_t irq, struct context *ctx);

#define VADDR(pdi, pti) ((uint32_t)(((pdi)<<PGDR_SHIFT)|((pti)<<PAGE_SHIFT)))

#define KERN_MAX_ADDR VADDR(1023, 1023)
#define KERN_MIN_ADDR VADDR(767, 767)
#define USER_MAX_ADDR VADDR(767, 0)
#define USER_MIN_ADDR VADDR(1, 0)

#define	KERNBASE  VADDR(768, 0)
#define R(x) ((x)-KERNBASE)

#define NR_KERN_PAGETABLE 20

extern uint8_t   end;
extern uint32_t *PT;
extern uint32_t *PTD;
#define vtopte(va) (PT+((va)>>PAGE_SHIFT))

struct callout {
	int32_t  expire;
	void 	 (*fun)(void *pv);
	void	 *pv;

  struct callout *next;
};

extern struct callout *g_callout_head;
extern void *g_sem_callout;
void  init_callout();
void *set_callout(int32_t expire, void (*fun)(void *), void *pv);
int   kill_callout(void *co);

void *sem_create(int32_t value);
int   sem_destroy(void *);
int   sem_wait(void *);
int   sem_signal(void *);

void  init_kmalloc(void *mem, size_t bytes);
void *kmalloc(size_t bytes);
void *krealloc(void *oldptr, size_t size);
void  kfree(void *ptr);

#define MEM_ZONE_LEN	(2 * 8)
extern uint32_t g_mem_zone[MEM_ZONE_LEN];

extern uint32_t g_kern_cur_addr;
extern uint32_t g_kern_end_addr;
extern uint8_t *g_frame_freemap;
extern uint32_t g_frame_count;
extern uint8_t *g_kern_heap_base;
extern uint32_t g_kern_heap_size;

struct tcb {
  /*hardcoded*/
  uint32_t        kern_stack;   /*saved top of the kernel stack for this task*/


  int32_t	        tid;     /* task id */
  int32_t         state;   /* -1:blocked, 0:running, 1:ready, 2:zombie */
#define TASK_STATE_BLOCKED 	-1
#define TASK_STATE_READY		 1
#define TASK_STATE_ZOMBIE		 2

  int32_t         quantum;
#define DEFAULT_QUANTUM 10

  int32_t         exit_code;

  struct wait_queue *wait_head;

  struct tcb     *all_next;
};

struct wait_queue {
  struct tcb *tsk;
  struct wait_queue *next;
};

void sleep_on(struct wait_queue **wq);
void wake_up(struct wait_queue **wq, int n);

extern struct tcb *g_task_running;
extern struct tcb *g_task_all_head;
extern struct tcb *task0;

extern int g_resched;
void schedule();
void switch_to(struct tcb *new);

void init_task(void);
int  sys_task_create(uint32_t user_stack, void (*handler)(void *), void *param);
void sys_task_exit(int val);
int  sys_task_wait(int32_t tid, int32_t *exit_code);
int32_t sys_task_getid();
void sys_task_yield();
int sys_task_sleep(uint32_t msec);
extern void *ret_from_syscall;

#define HZ   100
extern unsigned volatile ticks;
void isr_timer(uint32_t irq, struct context *ctx);

int do_page_fault(uint32_t vaddr, uint32_t code); 
void syscall(struct context *ctx); 

void init_floppy();
uint8_t *floppy_read_sector (uint32_t lba);

void init_fat();
int  fat_fopen(const char *pathname, int flag);
#define O_RDONLY		1
#define O_WRONLY		2
#define O_RDWR			(O_RDONLY | O_WRONLY)

int  fat_fclose(int fd);
int  fat_fgetsize(int fd);

#define EOF (-1)
int  fat_fread(int fd, void *buf, size_t count);

#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2
int  fat_fseek(int fd, unsigned int offset, int whence);

uint32_t load_pe(char *file);

#endif
