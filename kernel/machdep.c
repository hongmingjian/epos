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
#include <stddef.h>
#include <syscall-nr.h>
#include <ioctl.h>
#include <sys/mman.h>
#include <string.h>

#include "kernel.h"

/*计算机启动时，自1970-01-01 00:00:00 +0000 (UTC)以来的秒数*/
time_t g_startup_time;

/**
 * 初始化定时器
 */
static void init_pit(uint32_t freq)
{
    uint32_t timer_clock = 1000000;
    armtimer_reg_t *pit = (armtimer_reg_t *)(MMIO_BASE+ARMTIMER_REG_BASE);
    pit->Load = timer_clock/freq;
    pit->Reload = pit->Load;
    pit->PreDivider = (SYS_CLOCK_FREQ/timer_clock)-1;
    pit->Control = ARMTIMER_CTRL_23BIT |
                ARMTIMER_CTRL_PRESCALE_1 |
                ARMTIMER_CTRL_INTR_ENABLE |
                ARMTIMER_CTRL_ENABLE;
}

/**
 * 初始化中断控制器
 */
static void init_pic()
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE+INTR_REG_BASE);
    pic->Disable_basic_IRQs = 0xffffffff;
    pic->Disable_IRQs_1 = 0xffffffff;
    pic->Disable_IRQs_2 = 0xffffffff;
}

/**
 * 让中断控制器打开某个中断
 */
void enable_irq(uint32_t irq)
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE+INTR_REG_BASE);
    if(irq  < 8) {
        pic->Enable_basic_IRQs = 1 << irq;
    } else if(irq < 40) {
        pic->Enable_IRQs_1 = 1<<(irq - 8);
    } else if(irq < 72) {
        pic->Enable_IRQs_2 = 1<<(irq - 40);
    }
}

/**
 * 让中断控制器关闭某个中断
 */
void disable_irq(uint32_t irq)
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE+INTR_REG_BASE);
    if(irq  < 8) {
        pic->Disable_basic_IRQs = 1 << irq;
    } else if(irq < 40) {
        pic->Disable_IRQs_1 = 1<<(irq - 8);
    } else if(irq < 72) {
        pic->Disable_IRQs_2 = 1<<(irq - 40);
    }
}

/**
 * 把CPU从当前线程切换去运行线程new，即上下文切换（Context switch）
 */
void switch_to(struct tcb *new)
{

}

/**
 * 初始化串口
 */
static void init_uart(uint32_t baud)
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE+AUX_REG_BASE);
    gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE+GPIO_REG_BASE);

    aux->enables = 1;
    aux->mu_ier = 0;
    aux->mu_cntl = 0;
    aux->mu_lcr = 3;
    aux->mu_mcr = 0;
    aux->mu_baud = (SYS_CLOCK_FREQ/(8*baud))-1;

    uint32_t ra=gpio->gpfsel1;
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    ra&=~(7<<15); //gpio15
    ra|=2<<15;    //alt5
    gpio->gpfsel1 = ra;

    gpio->gppud = 0;
    gpio->gppudclk0 = (1<<14)|(1<<15);
    gpio->gppudclk0 = 0;

    aux->mu_cntl = 3;
}

void uart_putc ( int c )
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE+AUX_REG_BASE);
    while(1) {
        if(aux->mu_lsr&0x20)
            break;
    }
    aux->mu_io = c;
}

int uart_getc()
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE+AUX_REG_BASE);
    while(1) {
        if(aux->mu_lsr&0x1)
            break;
    }
    return aux->mu_io;
}

/**
 * 系统调用putchar的执行函数
 *
 * 往屏幕上的当前光标位置打印一个字符，相应地移动光标的位置
 */
int sys_putchar(int c)
{
    uart_putc(c);
    return c;
}

int exception(struct context *ctx)
{
    printk("  CPSR: 0x%x\r\n", ctx->cf_spsr);
    printk("  R0  : 0x%x\r\n", ctx->cf_r0);
    printk("  R1  : 0x%x\r\n", ctx->cf_r1);
    printk("  R2  : 0x%x\r\n", ctx->cf_r2);
    printk("  R3  : 0x%x\r\n", ctx->cf_r3);
    printk("  R4  : 0x%x\r\n", ctx->cf_r4);
    printk("  R5  : 0x%x\r\n", ctx->cf_r5);
    printk("  R6  : 0x%x\r\n", ctx->cf_r6);
    printk("  R7  : 0x%x\r\n", ctx->cf_r7);
    printk("  R8  : 0x%x\r\n", ctx->cf_r8);
    printk("  R9  : 0x%x\r\n", ctx->cf_r9);
    printk("  R10 : 0x%x\r\n", ctx->cf_r10);
    printk("  R11 : 0x%x\r\n", ctx->cf_r11);
    printk("  R12 : 0x%x\r\n", ctx->cf_r12);
    printk("  USR_SP  : 0x%x\r\n", ctx->cf_usr_sp);
    printk("  USR_LR  : 0x%x\r\n", ctx->cf_usr_lr);
    printk("  SVC_SP  : 0x%x\r\n", ctx->cf_svc_sp);
    printk("  SVC_LR  : 0x%x\r\n", ctx->cf_svc_lr);
    printk("  PC  : 0x%x\r\n", ctx->cf_pc);

	while(1);

	return 0;
}

void abort_handler(struct context *ctx, uint32_t vaddr, uint32_t code)
{
    if(do_page_fault(ctx, vaddr, code) < 0)
		exception(ctx);
}

void irq_handler(struct context *ctx)
{

    int irq;
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE+INTR_REG_BASE);

    for(irq = 0 ; irq < 8; irq++)
        if(pic->IRQ_basic_pending & (1<<irq))
            break;

    if(irq == 8){
        for( ; irq < 40; irq++)
            if(pic->IRQ_pending_1 & (1<<(irq-8)))
                break;

        if(irq == 40) {
            for( ; irq < 72; irq++)
                if(pic->IRQ_pending_1 & (1<<(irq-40)))
                    break;

            if(irq == 72)
                return;
        }
    }

    g_intr_vector[irq](irq, ctx);

    switch(irq) {
    case 0: {
        armtimer_reg_t *pit = (armtimer_reg_t *)(MMIO_BASE+ARMTIMER_REG_BASE);
        pit->IRQClear = 1;
        break;
    }
    }
}

void undefined_handler(struct context *ctx)
{
	printk("undefined exception\r\n");
    exception(ctx);
}

void swi_handler(struct context *ctx)
{
	printk("swi\r\n");
    exception(ctx);
}

/**
 * 系统调用分发函数，ctx保存了进入内核前CPU各个寄存器的值
 */
void syscall(struct context *ctx)
{
    //printk("task #%d syscalling #%d.\r\n", sys_task_getid(), ctx->cf_r0);
    switch(ctx->cf_r0) {
    case SYSCALL_task_exit:
    case SYSCALL_task_create:
    case SYSCALL_task_getid:
    case SYSCALL_task_yield:
    case SYSCALL_task_wait:
    case SYSCALL_reboot:
    case SYSCALL_mmap:
    case SYSCALL_munmap:
    case SYSCALL_sleep:
    case SYSCALL_nanosleep:
    case SYSCALL_beep:
    case SYSCALL_vm86:
    case SYSCALL_putchar:
    case SYSCALL_getchar:
    case SYSCALL_recv:
    case SYSCALL_send:
    case SYSCALL_ioctl:
    default:
        printk("syscall #%d not implemented.\r\n", ctx->cf_r0);
        break;
    }
}
/**
 * page fault处理函数。
 * 特别注意：此时系统的中断处于打开状态
 */
int do_page_fault(struct context *ctx, uint32_t vaddr, uint32_t code)
{
    uint32_t i, prot;

#if VERBOSE
    printk("PF:0x%08x(0x%04x)", vaddr, code);
#endif

    if(code & (1<<10)) {
#if !VERBOSE
        printk("PF:0x%08x(0x%04x)", vaddr, code);
#endif
        printk("->UNKNOWN CODE\r\n");
        return -1;
    } else {
        switch(code & 0xf) {
        case 0x3: //domain fault (page)
        case 0xf: //permission fault (page)
#if !VERBOSE
            printk("PF:0x%08x(0x%04x)", vaddr, code);
#endif
            printk("->PROTECTION VIOLATION\r\n");
            return -1;
            break;
        case 0x7: //translation fault (page)
            break;
        default:
#if !VERBOSE
            printk("PF:0x%08x(0x%04x)", vaddr, code);
#endif
            printk("->UNKNOWN CODE\r\n");
            return -1;
            break;
        }
    }

    /*检查地址是否合法*/
    prot = page_prot(vaddr);
    if(prot == -1 || prot == VM_PROT_NONE) {
#if !VERBOSE
        printk("PF:0x%08x(0x%04x)", vaddr, code);
#endif
        printk("->ILLEGAL MEMORY ACCESS\r\n");
        return -1;
    }

    {
        uint32_t paddr;
        uint32_t flags = L2E_V|L2E_C;

        if(prot & VM_PROT_WRITE)
            flags |= L2E_W;

        /*只要访问用户的地址空间，都代表用户模式访问*/
        if(vaddr < USER_MAX_ADDR)
            flags |= L2E_U;

        /*搜索空闲帧*/
        paddr = frame_alloc(1);
        if(paddr != SIZE_MAX) {
            /*找到空闲帧*/

			/*如果是小页表引起的缺页，需要填充页目录*/
			if(vaddr >= USER_MAX_ADDR && vaddr < KERNBASE) {
				for(i = 0; i < PAGE_SIZE/L2_TABLE_SIZE; i++) {
					PTD[i+((PAGE_TRUNCATE(vaddr)-USER_MAX_ADDR))/L2_TABLE_SIZE] = (paddr+i*L2_TABLE_SIZE)|L1E_V;
				}
			}

            *vtopte(vaddr) = paddr|flags;

            invlpg(vaddr);

            memset((void *)(PAGE_TRUNCATE(vaddr)), 0, PAGE_SIZE);

#if VERBOSE
            printk("->0x%08x\r\n", *vtopte(vaddr));
#endif

            return 0;
        } else {
            /*物理内存已耗尽*/
#if !VERBOSE
            printk("PF:0x%08x(0x%04x)", vaddr, code);
#endif
            printk("->OUT OF RAM\r\n");
        }
    }

    return -1;
}

/**
 * 初始化分页子系统
 */
static uint32_t init_paging(uint32_t physfree)
{
    uint32_t i;
    uint32_t *pgdir, *pte;

    /*
     * 分配页目录
     */
    pgdir=(uint32_t *)physfree;
    physfree += L1_TABLE_SIZE;
    memset(pgdir, 0, L1_TABLE_SIZE);

    /*
     * 分配4张小页表，并填充到页目录
     * 用于映射页表自身所占的虚拟内存范围，即[0xbfc00000, 0xc0000000]
     */
    uint32_t *ptpte = (uint32_t *)physfree;
    for(i = 0; i < PAGE_SIZE/L2_TABLE_SIZE; i++)
    {
        pgdir[i+(USER_MAX_ADDR>>PGDR_SHIFT)] = (physfree)|L1E_V;
        memset((void *)physfree, 0, L2_TABLE_SIZE);
        physfree+=L2_TABLE_SIZE;
    }

    /*
     * 分配NR_KERN_PAGETABLE张小页表，并填充到页目录，也填充到ptpte
     */
    pte = (uint32_t *)physfree;
    for(i = 0; i < NR_KERN_PAGETABLE; i++)
    {
        pgdir[i] = pgdir[i+(KERNBASE>>PGDR_SHIFT)] = (physfree)|L1E_V;

        if((i & 3) == 0)
            ptpte[3*L2_ENTRY_COUNT+(i>>2)] = (physfree)|L2E_V|L2E_W|L2E_C;

        memset((void *)physfree, 0, L2_TABLE_SIZE);
        physfree += L2_TABLE_SIZE;
    }
    ptpte[3*L2_ENTRY_COUNT-1] = ((uint32_t)ptpte)|L2E_V|L2E_W|L2E_C;

    /*
     * 设置恒等映射，填充小页表
     * 映射虚拟地址[0, 0x4000]和[KERNBASE, KERNBASE+0x4000]到物理地址为[0, 0x4000]
     */
    for(i = 0; i < 0x4000; i+=PAGE_SIZE)
      pte[i>>PAGE_SHIFT] = i|L2E_V|L2E_W|L2E_C;

    /*
     * 把页目录映射到[0x4000, 0x8000]和[KERNBASE+0x4000, KERNBASE+0x8000]
     */
    for(i = 0x4000; i < 0x8000; i+=PAGE_SIZE)
      pte[i>>PAGE_SHIFT] = (((uint32_t)pgdir)+i-0x4000)|L2E_V|L2E_W|L2E_C;

    /*
     * 设置恒等映射，填充小页表
     * 映射虚拟地址[0x8000, R(_end)]和[KERNBASE+0x8000, _end]到物理地址为[0x8000, R(_end)]
     */
    for(i = 0x8000; i < (uint32_t)pgdir; i+=PAGE_SIZE)
      pte[i>>PAGE_SHIFT] = i|L2E_V|L2E_W|L2E_C;

    /*
     * 打开分页
     */
	/* Translation table 0 */
	__asm__ __volatile__("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pgdir));
	/* Translation table 1 */
	__asm__ __volatile__("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pgdir));
	/* Use translation table 0 for everything, for now */
	__asm__ __volatile__("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (0));

	/* Set Domain 0 ACL to "Client", enforcing memory permissions
	 * See ARM1176JZF-S manual, 3-64
	 * Every mapped section/page is in domain 0
	 */
	__asm__ __volatile__("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (0x1));

	/* Read control register */
	register uint32_t control;
	__asm__ __volatile__("mrc p15, 0, %[control], c1, c0, 0" : [control] "=r" (control));
	/* Turn on MMU */
	control |= 1;
	/* Enable ARMv6 MMU features (disable sub-page AP) */
	control |= (1<<23);
	/* Write value back to control register */
	__asm__ __volatile__("mcr p15, 0, %[control], c1, c0, 0" : : [control] "r" (control));

    return physfree;
}

/**
 * 初始化物理内存
*/
static void init_ram(uint32_t physfree)
{
    g_ram_zone[0] = physfree;
    g_ram_zone[1] = 0x1FFFFFFF;
    g_ram_zone[2] = 0;
    g_ram_zone[3] = 0;
}

/**
 * 机器相关（Machine Dependent）的初始化
 */
static void md_startup(uint32_t mbi, uint32_t physfree)
{
    physfree=init_paging(physfree);
    init_ram(physfree);

    /*Physical addresses range from 0x20000000 to 0x20FFFFFF for peripherals*/
    page_map(MMIO_BASE, 0x20000000, 4096, L2E_V|L2E_W);

    init_pic();
    init_pit(HZ);
    init_uart(9600);
}

/**
 * 这个函数是内核的C语言入口，被entry.S调用
 */
void cstart(uint32_t magic, uint32_t mbi)
{
    uint32_t _end = ROUNDUP(R((uint32_t)(&end)), L1_TABLE_SIZE);

    /*
     * 机器相关（Machine Dependent）的初始化
     */
    md_startup(mbi, _end);

	__asm__ __volatile__("add sp, sp, %0" : : "r" (KERNBASE));

	/*
     * 机器无关（Machine Independent）的初始化
     */
    mi_startup();
}
