#ifndef _KERNEL_H
#define _KERNEL_H

#include "machdep.h"

#define HZ   100
extern void (*intr_vector[])(struct frame fr);

#define VADDR(pdi, pti) ((uint32_t)(((pdi)<<PGDR_SHIFT)|((pti)<<PAGE_SHIFT)))

#define KERN_MAX_ADDR VADDR(1023, 1023)
#define KERN_MIN_ADDR VADDR(767, 767)
#define USER_MAX_ADDR VADDR(767, 0)

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
  struct frame    context;
  int32_t	        tid;     /* task id */
  int32_t         state;   /* -1:blocked, 0:running, 1:ready, 2:zombie */
#define TASK_STATE_BLOCKED 	-1
#define TASK_STATE_READY		 1
#define TASK_STATE_ZOMBIE		 2

  int32_t         quantum;
#define DEFAULT_QUANTUM 10

  uint8_t        *stack_base;   /* stack base for this task */
  int32_t         exit_code;    /* exit code */

  int32_t         wait_cnt;
  struct tcb     *wait_head;
  struct tcb     *wait_next;

  struct tcb     *sem_next;

  struct tcb     *all_next;
};

extern struct tcb *g_task_running;
extern struct tcb *g_task_all_head;

extern int g_resched;
void schedule();
void switch_to(struct tcb *new);

void init_task(void);
int  task_create(void *stack_base, void (*handler)(void *), void *param);
void task_exit(int val);
int  task_wait(int32_t tid, int32_t *exit_code);
int32_t task_getid();
void task_yield();
void task_sleep(int32_t msec);

extern unsigned volatile ticks;
void isr_timer(struct frame fr);

int do_page_fault(uint32_t vaddr, uint32_t code); 

#endif
