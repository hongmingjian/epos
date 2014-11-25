/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
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

extern void (*g_intr_vector[])(uint32_t irq, struct context *ctx);
void disable_irq(uint32_t irq);
void enable_irq(uint32_t irq);

#define HZ   100
extern unsigned volatile g_timer_ticks;
void isr_timer(uint32_t irq, struct context *ctx);
struct tm {
  int tm_sec;         /* seconds */
  int tm_min;         /* minutes */
  int tm_hour;        /* hours */
  int tm_mday;        /* day of the month */
  int tm_mon;         /* month */
  int tm_year;        /* year */
  int tm_wday;        /* day of the week */
  int tm_yday;        /* day in the year */
  int tm_isdst;       /* daylight saving time */
};
time_t mktime(struct tm * tm);
extern time_t g_startup_time;

void isr_keyboard(uint32_t irq, struct context *ctx);

struct tcb {
  /*hardcoded*/
  uint32_t        kstack;   /*saved top of the kernel stack for this task*/


  int32_t         tid;     /* task id */
  int32_t         state;   /* -1:waiting, 0:running, 1:ready, 2:zombie */
#define TASK_STATE_WAITING  -1
#define TASK_STATE_READY     1
#define TASK_STATE_ZOMBIE    2

  int32_t         timeslice;
#define DEFAULT_TIMESLICE 4

  int32_t          code_exit;
  struct wait_queue *wq_exit;

  struct tcb     *next;
  struct x87      fpu;
};

extern struct tcb *g_task_running;
extern struct tcb *g_task_head;
extern struct tcb *task0;
extern struct tcb *g_task_own_fpu;

extern int g_resched;
void schedule();
void switch_to(struct tcb *new);

struct wait_queue {
  struct tcb *tsk;
  struct wait_queue *next;
};
void sleep_on(struct wait_queue **head);
void wake_up(struct wait_queue **head, int n);

void init_task(void);
void syscall(struct context *ctx);
extern void *ret_from_syscall;

/**
 * `VADDR' comes from FreeBSD
 */
#define VADDR(pdi, pti) ((uint32_t)(((pdi)<<PGDR_SHIFT)|((pti)<<PAGE_SHIFT)))

#define KERN_MAX_ADDR VADDR(1023, 1023)
#define KERN_MIN_ADDR VADDR(767, 767)
#define USER_MAX_ADDR VADDR(767, 0)
#define USER_MIN_ADDR VADDR(1, 0)

#define KERNBASE  VADDR(768, 0)
#define R(x) ((x)-KERNBASE)

#define NR_KERN_PAGETABLE 20

/**
 * `PT', `PTD' and `vtopte' come from FreeBSD
 */
extern uint8_t   end;
extern uint32_t *PT;
extern uint32_t *PTD;
#define vtopte(va) (PT+((va)>>PAGE_SHIFT))

void  init_kmalloc(void *mem, size_t bytes);
void *kmalloc(size_t bytes);
void *krealloc(void *oldptr, size_t size);
void  kfree(void *ptr);

#define RAM_ZONE_LEN (2 * 8)
extern uint32_t g_ram_zone[RAM_ZONE_LEN];

extern uint32_t g_kern_cur_addr;
extern uint32_t g_kern_end_addr;
extern uint8_t *g_frame_freemap;
extern uint32_t g_frame_count;
extern uint8_t *g_kern_heap_base;
extern uint32_t g_kern_heap_size;
int do_page_fault(struct context *ctx, uint32_t vaddr, uint32_t code);

#if USE_FLOPPY
void init_floppy();
uint8_t *floppy_read_sector (uint32_t lba);
int floppy_write_sector (size_t lba, uint8_t *buffer);
#else
void ide_init(uint16_t bus);
void ide_read_sector(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t *buf);
void ide_write_sector(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t *buf);
#endif

#include "dosfs.h"
uint32_t load_pe(VOLINFO *pvi, char *filename);

int     sys_putchar(int c);
int     sys_getchar();

struct tcb *sys_task_create(void *tos, void (*func)(void *pv), void *pv);
void        sys_task_exit(int code_exit);
int         sys_task_wait(int32_t tid, int32_t *pcode_exit);
int32_t     sys_task_getid();
void        sys_task_yield();

#endif /*_KERNEL_H*/
