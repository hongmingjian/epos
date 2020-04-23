/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2005, 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _ARCH_H
#define _ARCH_H

#include "config.h"

#define PSR_MODE_USR  0x10
#define PSR_MODE_FIQ  0x11
#define PSR_MODE_IRQ  0x12
#define PSR_MODE_SVC  0x13
#define PSR_MODE_MON  0x16
#define PSR_MODE_ABT  0x17
#define PSR_MODE_HYP  0x1a
#define PSR_MODE_UND  0x1b
#define PSR_MODE_SYS  0x1f
#define PSR_MODE_MASK 0x1f

#define PSR_A		  (1<<8)
#define PSR_I		  (1<<7)
#define PSR_F		  (1<<6)
#define PSR_T		  (1<<5)

#define _L1_TYPE_C       1         /* Coarse L2 */
#define _L1_C_NS         (1 << 3)  /* Non-secure */
#define _L1_C_DOM(x)     (((x) & 0xf) << 5)      /* Domain */

#define _L2_S_XN         1         /* Execute-Never */
#define _L2_TYPE_S       (1 << 1)  /* Small Page */
#define _L2_B            (1 << 2)  /* Bufferable page */
#define _L2_C            (1 << 3)  /* Cacheable page */
#define _L2_AP0          (1 << 4)  /* AP[0]:  Access permission bit 0 */
#define _L2_AP1          (1 << 5)  /* AP[1]:  Access permission bit 1 */
#define _L2_S_TEX(x)     (((x) & 0x7) << 6)
#define _L2_AP2          (1 << 9)  /* Bit 15: AP[2]:  Access permission bit 2 */
#define _L2_S            (1 << 10) /* Shared */
#define _L2_nG           (1 << 11) /* Not-Global */

#define ROUNDUP(x, y)    (((x)+((y)-1))&(~((y)-1)))
#define PAGE_MASK        (PAGE_SIZE-1)
#define PAGE_TRUNCATE(x) ((x)&(~PAGE_MASK))
#define PAGE_ROUNDUP(x)  ROUNDUP(x, PAGE_SIZE)

#define L1_ENTRY_COUNT   (1<<(32-PGDR_SHIFT))
#define L2_ENTRY_COUNT   (1<<(PGDR_SHIFT-PAGE_SHIFT))
#define L1_TABLE_SIZE    (4*L1_ENTRY_COUNT)
#define L2_TABLE_SIZE    (4*L2_ENTRY_COUNT)

#define L1E_V  (_L1_C_DOM(0)|_L1_TYPE_C) /* D0 = Client */
#define L1E_W  0
#define L1E_U  0
#define L1E_C  0

#define L2E_V  (_L2_S_TEX(0)|_L2_TYPE_S) /* Valid */
#define L2E_W  _L2_AP0                   /* Read/Write */
#define L2E_U  _L2_AP1                   /* User/Supervisor */
#define L2E_C  (_L2_B|_L2_C)             /* Cacheable */

#ifndef __ASSEMBLY__
#include <stdint.h>

#define save_flags_cli(flags)		\
	do {							\
		unsigned int tmp;			\
		__asm__ __volatile__(		\
		"mrs %0, cpsr\n\t"       	\
		"orr %1, %0, #0x80\n\t"		\
		"msr cpsr_cxsf, %1\n\t"		\
		:"=r"(flags), "=r"(tmp)		\
		:							\
		:"memory"					\
		);							\
	} while(0)

#define restore_flags(flags)		\
	do {							\
		__asm__ __volatile__(		\
		"msr cpsr_cxsf, %0\n\t"  	\
		:							\
		:"r"(flags)					\
		:"memory"					\
		);							\
	} while(0)

extern void sti(), cli();
extern void invlpg(uint32_t page);
extern void enable_l1_dcache();
extern void cpu_idle();
extern int64_t /*struct {int quot, int rem}*/\
       __aeabi_idivmod(int num, int den);
extern void atomic_or (unsigned long *, unsigned long),
            atomic_xor(unsigned long *, unsigned long),
            atomic_and(unsigned long *, unsigned long);
#endif /*__ASSEMBLY__*/

#endif /*_ARCH_H*/
