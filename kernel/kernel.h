/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
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

#include <sys/types.h>
#include <stdint.h>
#include <stdarg.h>

#include "machdep.h"

/*中断向量表*/
extern void (*g_intr_vector[])(uint32_t irq, struct context *ctx);

/*让中断控制器关闭某个中断*/
void disable_irq(uint32_t irq);

/*让中断控制器打开某个中断*/
void enable_irq(uint32_t irq);

/*定时器以HZ的频率中断CPU*/
#define HZ   100

/*记录系统启动以来，定时器中断的次数*/
extern unsigned volatile g_timer_ticks;

/*计算机启动时，自1970-01-01 00:00:00 +0000 (UTC)以来的秒数*/
extern time_t g_startup_time;

/**
 * 线程控制块
 *
 *       4 kB +---------------------------------+
 *            |          kernel stack           |
 *            |                |                |
 *            |                |                |
 *            |                V                |
 *            |         grows downward          |
 *            |                                 |
 *            |                                 |
 *            |                                 |
 *            |                                 |
 *            |                                 |
 *            |                                 |
 *            |                                 |
 *            |                                 |
 *            +---------------------------------+
 *            |            signature            |
 *            |                :                |
 *            |                :                |
 *            |               tid               |
 *            |              kstack             |
 *       0 kB +---------------------------------+
 */
struct tcb {
    /*hardcoded*/
    uint32_t    kstack;      /*saved top of the kernel stack for this task*/

    int         tid;         /* task id */
    int         state;       /* -1:waiting,0:running,1:ready,2:zombie */
#define TASK_STATE_WAITING  -1
#define TASK_STATE_READY     1
#define TASK_STATE_ZOMBIE    2

    int         timeslice;   //时间片
#define TASK_TIMESLICE_DEFAULT 4

    int         code_exit;   //保存该线程的退出代码
    struct wait_queue *wq_exit; //等待该线程退出的队列

    struct tcb  *next;

    uint32_t     signature;  //必须是最后一个字段
#define TASK_SIGNATURE 0x20160201
};

#define TASK_KSTACK 0           /*=offsetof(struct tcb, kstack)*/

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
struct tcb *sys_task_create(void *tos, void (*func)(void *pv), void *pv);
void        sys_task_exit(int code_exit);
int         sys_task_wait(int tid, int *pcode_exit);
int         sys_task_getid();
void        sys_task_yield();

/**
 * `VADDR' comes from FreeBSD
 */
#define VADDR(pdi, pti) ((uint32_t)(((pdi)<<PGDR_SHIFT)|((pti)<<PAGE_SHIFT)))

#define KERN_MAX_ADDR VADDR(0xFFF, 0xFF)
#define KERN_MIN_ADDR VADDR(0xC00, ((LOADADDR-L1_TABLE_SIZE)>>PAGE_SHIFT))
#define USER_MAX_ADDR VADDR(0xBFC, 0x00)
#define USER_MIN_ADDR VADDR(4, 0)

//#define KERNBASE  VADDR(0xC00, 0)

#define IN_USER_VM(va, len) \
       ((uint32_t)(va) >= USER_MIN_ADDR && \
        (uint32_t)(va) <  USER_MAX_ADDR && \
        (uint32_t)(va) + (len) <= USER_MAX_ADDR)

/**
 * `PT', `PTD' and `vtopte' come from FreeBSD
 */
extern uint8_t   end;
extern uint32_t *PT;
extern uint32_t *PTD;
#define vtopte(va) (PT+((va)>>PAGE_SHIFT))
#define vtop(va) (((*vtopte(va))&(~PAGE_MASK))|((va)&PAGE_MASK))

void  init_kmalloc(void *mem, size_t bytes);
void *kmalloc(size_t bytes);
void *krealloc(void *oldptr, size_t size);
void  kfree(void *ptr);
void *kmemalign(size_t align, size_t bytes);

#define RAM_ZONE_LEN (2 * 8)
extern uint32_t g_ram_zone[RAM_ZONE_LEN];

int do_page_fault(struct context *ctx, uint32_t vaddr, uint32_t code);

int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
int printk(const char *fmt,...);

struct file;
struct vmzone {
    uint32_t base;
    uint32_t limit;
    int prot;

    struct file *fp;
    off_t offset;
    int flags;

    struct vmzone *next;
};
void init_vmspace(uint32_t brk);
struct vmzone *page_alloc(int npages, int prot, int user,
                          int flags, struct file *fp, off_t offset);
struct vmzone *page_alloc_in_addr(uint32_t va, int npages, int prot,
                                  int flags, struct file *fp, off_t offset);
struct vmzone *page_zone(uint32_t va);
int            page_free(uint32_t va, int npages);
#define VM_PROT_NONE   0x00
#define VM_PROT_READ   0x01
#define VM_PROT_WRITE  0x02
#define VM_PROT_EXEC   0x04
#define VM_PROT_RW     (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL    (VM_PROT_RW  |VM_PROT_EXEC)

void     page_map(uint32_t vaddr, uint32_t paddr, uint32_t npages, uint32_t flags);
void     page_unmap(uint32_t vaddr, uint32_t npages);

#define	PROT_NONE	0x00	/* [MC2] no permissions */
#define	PROT_READ	0x01	/* [MC2] pages can be read */
#define	PROT_WRITE	0x02	/* [MC2] pages can be written */
#define	PROT_EXEC	0x04	/* [MC2] pages can be executed */

#define	MAP_SHARED	0x0001		/* [MF|SHM] share changes */
#define	MAP_PRIVATE	0x0002		/* [MF|SHM] changes are private */
#define	MAP_FIXED	0x0010		/* interpret addr exactly */
#define	MAP_FILE	0x0000		/* map from file (default) */
#define	MAP_ANON	0x1000		/* allocated from memory, swap space */
#define MAP_STACK   0x20000

#define MAP_FAILED	((void *)-1)	/* [MF|SHM] mmap failed */

uint32_t init_frame(uint32_t brk);
uint32_t frame_alloc(uint32_t npages);
uint32_t frame_alloc_in_addr(uint32_t pa, uint32_t npages);
void     frame_free(uint32_t paddr, uint32_t npages);

void     calibrate_delay(void);
unsigned sys_sleep(unsigned seconds);

struct timespec
{
    time_t tv_sec;      /* seconds */
    long tv_nsec;       /* nanoseconds */
};
int sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

struct timeval {
	long tv_sec;        /* seconds */
	long tv_usec;       /* microseconds */
};
int sys_gettimeofday(struct timeval *tv, void *tzp);

void mi_startup();

struct dev;
struct driver {
	char *major;
	int (*attach)(struct dev *dp);
   void (*detach)(struct dev *dp);
	int (*read)  (struct dev *dp, uint32_t addr, uint8_t *buf, size_t size);
	int (*write) (struct dev *dp, uint32_t addr, uint8_t *buf, size_t size);
	int (*poll)  (struct dev *dp, short events);
#define POLLIN		0x0001
#define POLLOUT		0x0004

	int (*ioctl) (struct dev *dp, uint32_t cmd, void *arg);
};

struct dev {
	struct driver *drv;
	uint32_t minor;
};

#define NR_DEVICE 16
extern struct dev    *g_dev_vector[];

struct file;
struct fs {
	int (*mount)  (struct fs *this, struct dev *dev, uint32_t addr);
	int (*unmount)(struct fs *this);
	int (*open)   (struct fs *this, char *path, int mode, struct file **_fpp);
#define O_RDONLY	0x0
#define O_WRONLY	0x1
#define O_RDWR      0x2
#define O_APPEND	0x2000

	int (*close)  (struct file *_fp);
	int (*read)   (struct file *_fp, uint8_t *buf, size_t size);
	int (*write)  (struct file *_fp, uint8_t *buf, size_t size);
	int (*seek)   (struct file *_fp, off_t offset, int whence);
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

	int (*poll)   (struct file *_fp, short events);
	int (*ioctl)  (struct file *_fp, uint32_t cmd, void *arg);
};
struct file {
	struct fs *fs;
	int refcnt;
};

#define NR_FILE_SYSTEM   16
#define NR_OPEN_FILE 64
extern struct fs *g_fs_vector[];
extern struct file *g_file_vector[];

int sys_open(char *path, int mode);
int sys_close(int fd);
int sys_read(int fd, uint8_t *buffer, size_t size);
int sys_write(int fd, uint8_t *buffer, size_t size);
int sys_seek(int fd, off_t offset, int whence);
int sys_ioctl(int fd, uint32_t cmd, void *arg);

#endif /*_KERNEL_H*/
