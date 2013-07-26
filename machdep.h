#ifndef _MACHDEP_H
#define _MACHDEP_H

#include "cpu.h"

#define NR_IRQ        16
#define IRQ_TIMER     0
#define IRQ_KEYBOARD  1

struct frame {
	uint32_t	edi;
	uint32_t	esi;
	uint32_t	ebp;
	uint32_t	isp;
	uint32_t	ebx;
	uint32_t	edx;
	uint32_t	ecx;
	uint32_t	eax;
	uint32_t	trapno;
	uint32_t	code;
	/* below portion defined in 386 hardware */
	uint32_t	eip;
	uint32_t	cs;
	uint32_t	eflags;
	/* below only when crossing rings (e.g. user to kernel) */
	uint32_t	esp;
	uint32_t	ss;
};

#define INIT_CONTEXT(ctx, stack_base, handler) do { \
  ctx.edi = 0x11111111; \
  ctx.esi = 0x22222222; \
  ctx.ebp = 0x33333333; \
  ctx.isp = (uint32_t)(stack_base); \
  ctx.ebx = 0x55555555; \
  ctx.edx = 0x66666666; \
  ctx.ecx = 0x77777777; \
  ctx.eax = 0x88888888; \
  ctx.eip = (uint32_t)(handler); \
  ctx.cs = 0x8/*XXX*/; \
  ctx.eflags = 0x202/*XXX*/; \
} while(0) 

#define PUSH_CONTEXT_STACK(ctx, value) do { \
  ctx.isp -= 4;  \
  *((uint32_t *)ctx.isp) = (uint32_t)(value); \
} while(0)

void disable_irq(uint32_t irq);
void enable_irq(uint32_t irq);

int  putchar(int c);
void init_machdep(uint32_t physfree);
#endif
