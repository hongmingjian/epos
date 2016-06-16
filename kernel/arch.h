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

#include <inttypes.h>

#define PSR_MODE_USR  0x10
#define PSR_MODE_FIQ  0x11
#define PSR_MODE_IRQ  0x12
#define PSR_MODE_SVC  0x13
#define PSR_MODE_ABT  0x17
#define PSR_MODE_UND  0x1b
#define PSR_MODE_SYS  0x1f
#define PSR_MODE_MASK 0x1f

#define PSR_N		  0x80000000
#define PSR_Z		  0x40000000
#define PSR_C		  0x20000000
#define PSR_V		  0x10000000
#define PSR_I		  0x80
#define PSR_F		  0x40
#define PSR_T		  0x20

static __inline void
cpu_idle()
{
    __asm__ __volatile__("mov 	r0, r0");
}

#define save_flags_cli(flags)					\
	do {										\
		unsigned int tmp;						\
		__asm__ __volatile__(					\
		"mrs	%0, cpsr\n\t"                   \
		"orr	%1, %0, #0x80\n\t"	            \
		"msr	cpsr_cxsf, %1\n\t"	            \
		:"=r"(flags), "=r"(tmp)					\
		:										\
		:"memory"								\
		);										\
	} while(0)

#define restore_flags(flags)					\
	do {										\
		__asm__ __volatile__(					\
		"msr	cpsr_cxsf, %0\n\t"              \
		:										\
		:"r"(flags)								\
		:"memory"								\
		);										\
	} while(0)

extern void invlpg(uint32_t page);

#define L1_TABLE_SIZE      0x4000 /* 16K */
#define L2_TABLE_SIZE      0x400  /* 1K */
#define L1_ENTRY_COUNT     (L1_TABLE_SIZE / 4)
#define L2_ENTRY_COUNT     (L2_TABLE_SIZE / 4)

#define _L1_TYPE_C       0x01      /* Coarse L2 */
#define _L1_C_NS         (1 << 3)  /* Non-secure */
#define _L1_C_DOM(x)     (((x) & 0xf) << 5)      /* domain */

#define _L2_S_XN         0x01
#define _L2_TYPE_S       0x02      /* Small Page */
#define _L2_B            0x04      /* Bufferable page */
#define _L2_C            0x08      /* Cacheable page */
#define _L2_AP0(x)       (((x) & 0x3) << 4)      /* access permissions (sp 0) */
#define _L2_S_TEX(x)     (((x) & 0x7) << 6)
#define _L2_APX          (1 << 9)
#define _L2_S            (1 << 10)
#define _L2_nG           (1 << 11)

#define ROUNDUP(x, y) (((x)+((y)-1))&(~((y)-1)))

#define PAGE_SHIFT  12
#define PGDR_SHIFT  20
#define PAGE_SIZE   (1<<PAGE_SHIFT)
#define PAGE_MASK   (PAGE_SIZE-1)
#define PAGE_TRUNCATE(x)  ((x)&(~PAGE_MASK))
#define PAGE_ROUNDUP(x)   ROUNDUP(x, PAGE_SIZE)

#define L1E_V   (_L1_C_DOM(0)|_L1_TYPE_C)
#define L1E_W   0
#define L1E_U   0
#define L1E_C   0

#define L2E_V   (_L2_S_TEX(0)|_L2_TYPE_S) /* Valid */
#define L2E_W   _L2_AP0(1)                /* Read/Write */
#define L2E_U   _L2_AP0(2)                /* User/Supervisor */
#define L2E_C   (_L2_B|_L2_C)             /* Cacheable */

#endif /*_ARCH_H*/
