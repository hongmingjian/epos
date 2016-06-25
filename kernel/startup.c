/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2015 Hong MingJian<hongmingjian@gmail.com>
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
#include <stddef.h>
#include "kernel.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*可用的物理内存区域*/
uint32_t g_ram_zone[RAM_ZONE_LEN];

uint32_t *PT  = (uint32_t *)USER_MAX_ADDR, //页表的指针
         *PTD = (uint32_t *)KERN_MIN_ADDR; //页目录的指针

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    //printk("IRQ=0x%02x\r\n", irq);
}

/**
 * 机器无关（Machine Independent）的初始化
 */
void mi_startup()
{
    uint32_t i, brk;

    printk("Welcome to EPOS\r\n");
    printk("Copyright (C) 2005-2015 MingJian Hong<hongmingjian@gmail.com>\r\n");
    printk("All rights reserved.\r\n\r\n");

    {
        /*安装默认的中断处理程序*/
        for(i = 0; i < NR_IRQ; i++)
            g_intr_vector[i]=isr_default;

        /*安装定时器的中断处理程序*/
        g_intr_vector[IRQ_TIMER] = isr_timer;
        enable_irq(IRQ_TIMER);
    }

    /*
     * 初始化物理内存管理器
     */
    brk = PAGE_ROUNDUP( (uint32_t)(&end) );
    brk = init_frame(brk);

    /*
     * 初始化虚拟地址空间，为内核堆预留4MiB的地址空间
     */
    init_vmspace(brk+1024*PAGE_SIZE);

    /*
     * 初始化内核堆，大小为4MiB，由kmalloc/kfree管理.
     */
    init_kmalloc((uint8_t *)brk, 1024*PAGE_SIZE);

	/*
	 * 把memory-mapped I/O所映射的虚拟内存保留下来
	 */
	page_alloc_in_addr(MMIO_BASE, 4096, VM_PROT_RW);

	/*
	 * 把异常向量remap到0xffff0000
	 */
	page_alloc_in_addr(0xffff0000, 1, VM_PROT_RW);
	page_map(0xffff0000, 0x0, 1, L2E_V|L2E_W|L2E_C);
    __asm__ __volatile__ (
             "mrc p15,0,r0,c1,c0,0\n\t"
             "orr r0, r0, #(1<<13) @ SCTLR.V=1\n\t"
             "mcr p15,0,r0,c1,c0,0\n\t"
             :
             :
             : "r0"
    );

	/*
	 * 取消恒等映射
	 */
	for(i = 0; i < NR_KERN_PAGETABLE; i++)
    {
		PTD[i] = 0;
	}

	/* Invalidate the translation lookaside buffer (TLB)
	 * ARM1176JZF-S manual, p. 3-86
	 */
	__asm__ __volatile__("mcr p15, 0, %[data], c8, c7, 0" : : [data] "r" (0));

	printk("We are in a virutalised world!");

	sti();

    while(1)
        cpu_idle();
}

