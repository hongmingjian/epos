#ifndef _MACHDEP_H
#define _MACHDEP_H

#include "cpu.h"

#define IRQ_TIMER     0
#define IRQ_KEYBOARD  1
#define IRQ_FDC       6
#define NR_IRQ        16

struct context {
  uint32_t   gs;/*0*/
  uint32_t   fs;/*4*/
  uint32_t   es;/*8*/
  uint32_t   ds;/*12*/
  uint32_t	edi;/*16*/
  uint32_t	esi;/*20*/
  uint32_t	ebp;/*24*/
  uint32_t	isp;/*28*/
  uint32_t	ebx;/*32*/
  uint32_t	edx;/*36*/
  uint32_t	ecx;/*40*/
  uint32_t	eax;/*44*/
  /* below portion defined in 386 hardware */
  uint32_t	eip;/*48*/
  uint32_t	 cs;/*52*/
  uint32_t	eflags;/*56*/
  /* below only when crossing rings (e.g. user to kernel) */
  uint32_t	esp;/*60*/
  uint32_t	 ss;/*64*/
};

#define PUSH_TASK_STACK(sp, value) do { \
  (sp)-=4; \
  *((uint32_t *)(sp)) = (uint32_t)(value); \
} while(0)

#define INIT_TASK_CONTEXT(user, kern_stack, user_stack, handler) do { \
  PUSH_TASK_STACK(kern_stack, (user)?0x23:0x10); \
  PUSH_TASK_STACK(kern_stack, user_stack); \
  PUSH_TASK_STACK(kern_stack, 0x202); \
  PUSH_TASK_STACK(kern_stack, (user)?0x1b:0x8); \
  PUSH_TASK_STACK(kern_stack, handler); \
  PUSH_TASK_STACK(kern_stack, 0x11111111); \
  PUSH_TASK_STACK(kern_stack, 0x22222222); \
  PUSH_TASK_STACK(kern_stack, 0x33333333); \
  PUSH_TASK_STACK(kern_stack, 0x44444444); \
  PUSH_TASK_STACK(kern_stack, 0x55555555); \
  PUSH_TASK_STACK(kern_stack, 0x66666666); \
  PUSH_TASK_STACK(kern_stack, 0x77777777); \
  PUSH_TASK_STACK(kern_stack, 0x88888888); \
  PUSH_TASK_STACK(kern_stack, (user)?0x23:0x10); \
  PUSH_TASK_STACK(kern_stack, (user)?0x23:0x10); \
  PUSH_TASK_STACK(kern_stack, (user)?0x23:0x10); \
  PUSH_TASK_STACK(kern_stack, (user)?0x23:0x10); \
  PUSH_TASK_STACK(kern_stack, &ret_from_syscall); \
} while(0)


void disable_irq(uint32_t irq);
void enable_irq(uint32_t irq);

int  putchar(int c);
void init_machdep(uint32_t physfree);
#endif
