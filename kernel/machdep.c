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
#include <string.h>

#include "kernel.h"

uint32_t cpuid, boardid;

/**
 * 初始化定时器
 */
static void init_pit(uint32_t freq)
{
#if IRQ_TIMER == 0
    uint32_t timer_clock = 1000000;
    armtimer_reg_t *pit = (armtimer_reg_t *)(MMIO_BASE_VA+ARMTIMER_REG);
    pit->Load = timer_clock/freq;
    pit->Reload = pit->Load;
    pit->PreDivider = (SYS_CLOCK_FREQ/timer_clock)-1;
    pit->Control = ARMTIMER_CTRL_23BIT |
                ARMTIMER_CTRL_PRESCALE_1 |
                ARMTIMER_CTRL_INTR_ENABLE |
                ARMTIMER_CTRL_ENABLE;
#else
/*
 * [1] https://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-System-Timer.html
 * [2] https://jsandler18.github.io/extra/sys-time.html
 * [3] https://github.com/jncronin/rpi-boot
 * [4] http://www.airspayce.com/mikem/bcm2835/
 * [5] https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=72260
 */
 	systimer_reg_t *pst = (systimer_reg_t *)(MMIO_BASE_VA+SYSTIMER_REG);
	pst->c1 = pst->clo + 1000000/HZ;
#endif
}

/**
 * 初始化中断控制器
 */
static void init_pic()
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);
    pic->Disable_basic_IRQs = 0xffffffff;
    pic->Disable_IRQs_1 = 0xffffffff;
    pic->Disable_IRQs_2 = 0xffffffff;
}

/**
 * 让中断控制器打开某个中断
 */
void enable_irq(uint32_t irq)
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);
    if(irq  < 8) {
        pic->Enable_basic_IRQs = 1 << irq;
    } else if(irq < 40) {
        pic->Enable_IRQs_1 = 1<<(irq - 8);
    } else if(irq < NR_IRQ) {
        pic->Enable_IRQs_2 = 1<<(irq - 40);
    }
}

/**
 * 让中断控制器关闭某个中断
 */
void disable_irq(uint32_t irq)
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);
    if(irq  < 8) {
        pic->Disable_basic_IRQs = 1 << irq;
    } else if(irq < 40) {
        pic->Disable_IRQs_1 = 1<<(irq - 8);
    } else if(irq < NR_IRQ) {
        pic->Disable_IRQs_2 = 1<<(irq - 40);
    }
}

/**
 * 把CPU从当前线程切换去运行线程new，即上下文切换（Context switch）
 */
void switch_to(struct tcb *new)
{
    __asm__ __volatile__ (
			"mrs r12, cpsr\n\t"
            "stmdb sp!, {r4-r12,r14}\n\t"
            "ldr r12, =1f\n\t"
            "stmdb sp!, {r12}\n\t"
            "ldr r12, %0\n\t"
            "str sp, [r12]\n\t"
            :
            :"m"(g_task_running)
            );

    g_task_running = new;

    __asm__ __volatile__ (
            "ldr r12, %0\n\t"
            "ldr sp, [r12]\n\t"
            "ldmia sp!, {pc}\n\t"
            "1:\n\t"
            "ldmia sp!, {r4-r12,r14}\n\t"
            "msr cpsr_cxsf, r12\n\t"
            :
            :"m"(g_task_running)
            );
}

/**
 * 初始化串口(Mini UART)
 */
void init_uart1(uint32_t baud)
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
    gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);

    aux->enables = 1; //Enable Mini UART
    aux->mu_cntl = 0; //Disable transmitter & receiver
    aux->mu_lcr = 3;  //8 data bits
    aux->mu_baud = (SYS_CLOCK_FREQ/(8*baud))-1;

    uint32_t ra=gpio->gpfsel1;
    ra &= ~((7 << 12) | (7 << 15)); // gpio14/gpio15
    ra |= (2 << 12) | (2 << 15);    // alt5=TxD1/RxD1
    gpio->gpfsel1 = ra;

	gpio->gppud = 0; //Disable pull-up/down
    ra = 150; while(ra--) __asm__ __volatile__("nop");
    gpio->gppudclk0 = (1 << 14) | (1 << 15);
    ra = 150; while(ra--) __asm__ __volatile__("nop");
    gpio->gppudclk0 = 0;

    aux->mu_cntl = 3; //Enable transmitter & receiver
}

void uart1_putc ( int c )
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
    while(1) {
        if(aux->mu_lsr&0x20) //Transmitter empty?
            break;
    }
    aux->mu_io = c & 0xff;
}

int uart1_hasc()
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
	return aux->mu_lsr&0x1;
}

int uart1_getc()
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
    while(1) {
        if(aux->mu_lsr&0x1)
            break;
    }
    return aux->mu_io & 0xff;
}

/**
 * 初始化串口(PL011)
 *    [1] https://qiita.com/fireflower0/items/602130c0ff0625f62ddc
 *    [2] https://www.raspberrypi.org/documentation/configuration/uart.md
 */
void init_uart0(uint32_t baud)
{
	uart_reg_t *uart = (uart_reg_t *)(MMIO_BASE_VA+UART_REG);
    gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);

    uart->cr/*PL011_CR*/ = 0;

    uint32_t ra = gpio->gpfsel1/*GPFSEL1*/;
    ra &= ~((7 << 12) | (7 << 15)); // gpio14/gpio15
    ra |= (4 << 12) | (4 << 15);    // alt0=TxD0/RxD0
    gpio->gpfsel1/*GPFSEL1*/ = ra;

    gpio->gppud/*GPPUD*/ = 0;
    ra = 150; while(ra--) __asm__ __volatile__("nop");
    gpio->gppudclk0/*GPPUDCLK0*/ = (1 << 14) | (1 << 15);
    ra = 150; while(ra--) __asm__ __volatile__("nop");
    gpio->gppudclk0/*GPPUDCLK0*/ = 0;

    uart->icr/*PL011_ICR*/  = 0x7FF;

	/*
     * Set integer & fractional part of baud rate.
	 * Divider = UART_CLOCK/(16 * Baud)
	 * Fraction part register = (Fractional part * 64) + 0.5
	 * UART_CLOCK = 3000000; Baud = 115200.

	 * Divider = 3000000 / (16 * 115200) = 1.627
	 * Integer part = 1
	 * Fractional part register = (.627 * 64) + 0.5 = 40.6 = 40
     */
    uart->ibrd/*PL011_IBRD*/ =  FUARTCLK/(16*baud);
    uart->fbrd/*PL011_FBRD*/ = (FUARTCLK%(16*baud))/(baud/4);

    uart->lcrh/*PL011_LCRH*/ = 0b11 << 5; // 8N1
    uart->cr/*PL011_CR*/   = 0x301;
}

void uart0_putc ( int c )
{
	uart_reg_t *uart = (uart_reg_t *)(MMIO_BASE_VA+UART_REG);
    do {
        __asm__ __volatile__("nop");
    } while(uart->fr/*PL011_FR*/ & 0x20);
    uart->dr/*PL011_DR*/ = c & 0xff;
}

int uart0_hasc()
{
	uart_reg_t *uart = (uart_reg_t *)(MMIO_BASE_VA+UART_REG);
	return uart->fr&0x10;
}

int uart0_getc()
{
	uart_reg_t *uart = (uart_reg_t *)(MMIO_BASE_VA+UART_REG);
    while(1) {
        if(uart->fr&0x10)
            break;
    }
    return uart->dr/*PL011_DR*/ & 0xff;
}

void init_uart(uint32_t baud)
{
	switch(cpuid) {
#if RPI_QEMU == 1
	case CPUID_QEMU:
		/*
		 * QEMU只模拟了PL011
		 */
		init_uart0(baud);
		break;
#endif
	case CPUID_BCM2711:
		/*
		 * 在Pi 4中, PL011连到蓝牙上了
		 * 只能用Mini UART
		 */
	default:
		init_uart1(baud);
		break;
	}
}

/**
 * 往屏幕上的当前光标位置打印一个字符，相应地移动光标的位置
 */
int putchar(int c)
{
	switch(cpuid) {
#if RPI_QEMU == 1
	case CPUID_QEMU:
		/*
		 * QEMU只模拟了PL011
		 */
		uart0_putc(c);
		break;
#endif
	case CPUID_BCM2711:
		/*
		 * 在Pi 4中, PL011连到蓝牙上了
		 * 只能用Mini UART
		 */
	default:
		uart1_putc(c);
		break;
	}
    return c;
}

int exception(struct context *ctx)
{
    printk("SPSR  : 0x%08x\r\n", ctx->cf_spsr);
    printk("R0    : 0x%08x\r\n", ctx->cf_r0);
    printk("R1    : 0x%08x\r\n", ctx->cf_r1);
    printk("R2    : 0x%08x\r\n", ctx->cf_r2);
    printk("R3    : 0x%08x\r\n", ctx->cf_r3);
    printk("R4    : 0x%08x\r\n", ctx->cf_r4);
    printk("R5    : 0x%08x\r\n", ctx->cf_r5);
    printk("R6    : 0x%08x\r\n", ctx->cf_r6);
    printk("R7    : 0x%08x\r\n", ctx->cf_r7);
    printk("R8    : 0x%08x\r\n", ctx->cf_r8);
    printk("R9    : 0x%08x\r\n", ctx->cf_r9);
    printk("R10   : 0x%08x\r\n", ctx->cf_r10);
    printk("R11   : 0x%08x\r\n", ctx->cf_r11);
    printk("R12   : 0x%08x\r\n", ctx->cf_r12);
    printk("USR_SP: 0x%08x\r\n", ctx->cf_usr_sp);
    printk("USR_LR: 0x%08x\r\n", ctx->cf_usr_lr);
    printk("SVC_SP: 0x%08x\r\n", ctx->cf_svc_sp);
    printk("SVC_LR: 0x%08x\r\n", ctx->cf_svc_lr);
    printk("PC    : 0x%08x\r\n", ctx->cf_pc);

    while(1);

    return 0;
}

void abort_handler(struct context *ctx, uint32_t far, uint32_t fsr)
{
    if(do_page_fault(ctx, far, fsr) == 0)
        return;

    cli();
    exception(ctx);
}

void irq_handler(struct context *ctx)
{
    int irq;
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);

    for(irq = 0 ; irq < 8; irq++)
        if(pic->IRQ_basic_pending & (1<<irq))
            break;

    if(irq == 8) {
        for( ; irq < 40; irq++)
            if(pic->IRQ_pending_1 & (1<<(irq-8)))
                break;

        if(irq == 40) {
            for( ; irq < NR_IRQ; irq++)
                if(pic->IRQ_pending_2 & (1<<(irq-40)))
                    break;

            if(irq == NR_IRQ)
                return;
        }
    }

    switch(irq) {
    case 0: {
        armtimer_reg_t *pit = (armtimer_reg_t *)(MMIO_BASE_VA+ARMTIMER_REG);
        pit->IRQClear = 1;
        break;
		}
	case 9: {
		systimer_reg_t *pst = (systimer_reg_t *)(MMIO_BASE_VA+SYSTIMER_REG);
		pst->cs |= (1<<1);
		pst->c1 = pst->clo + 1000000/HZ;
		break;
		}
    }

    sti();
    g_intr_vector[irq](irq, ctx);
	cli();
}

void undefined_handler(struct context *ctx)
{
    printk("undefined exception\r\n");
    exception(ctx);
}

/**
 * 系统调用分发函数，ctx保存了进入内核前CPU各个寄存器的值
 */
void syscall(struct context *ctx)
{
    //printk("task #%d syscalling #%d.\r\n", sys_task_getid(), ctx->cf_r4);
    switch(ctx->cf_r4) {
    case SYSCALL_task_exit:
        sys_task_exit(ctx->cf_r0);
        break;
    case SYSCALL_task_create:
        {
            uint32_t user_stack = ctx->cf_r0;
            uint32_t user_entry = ctx->cf_r1;
            uint32_t user_pvoid = ctx->cf_r2;
            struct tcb *tsk;

            //printk("stack: 0x%08x, entry: 0x%08x, pvoid: 0x%08x\r\n",
            //       user_stack, user_entry, user_pvoid);
            if(!IN_USER_VM(user_stack, 0) ||
               !IN_USER_VM(user_entry, 0)) {
                ctx->cf_r0 = -1;
                break;
            }
            tsk = sys_task_create((void *)user_stack,
                                  (void (*)(void *))user_entry,
                                  (void *)user_pvoid);
            ctx->cf_r0 = (tsk==NULL)?-1:tsk->tid;
        }
        break;
    case SYSCALL_task_getid:
        ctx->cf_r0=sys_task_getid();
        break;
    case SYSCALL_task_yield:
        sys_task_yield();
        break;
    case SYSCALL_task_wait:
        {
            int tid    = ctx->cf_r0;
            int *pcode = (int *)(ctx->cf_r1);
            if((pcode != NULL) && !IN_USER_VM(pcode, sizeof(int))) {
                ctx->cf_r0 = -1;
                break;
            }

            ctx->cf_r0 = sys_task_wait(tid, pcode);
        }
        break;
    case SYSCALL_mmap:
		{
            void *addr = (void *)(ctx->cf_r0);
            size_t len = ctx->cf_r1, len1=PAGE_ROUNDUP(len);
            int prot = ctx->cf_r2;
            int flags = ctx->cf_r3;
            int fd = *(int *)(ctx->cf_usr_sp+4);
            struct file *fp = NULL;
            off_t offset = *(off_t *)(ctx->cf_usr_sp+8);

            ctx->cf_r0 = (uint32_t)MAP_FAILED;
            if(fd >= 0) {
                if(fd >= NR_OPEN_FILE || g_file_vector[fd] == NULL)
					break;
				fp = g_file_vector[fd];
            }
		    ctx->cf_r0 = sys_mmap((uint32_t)addr, len1/PAGE_SIZE, prot, 1, flags, fp, offset);
		}
        break;
    case SYSCALL_munmap:
        {
            void *addr = (void *)(ctx->cf_r0);
            size_t len = ctx->cf_r1, len1=PAGE_ROUNDUP(len);
			ctx->cf_r0 = sys_munmap((uint32_t)addr, len1/PAGE_SIZE);
        }
        break;
    case SYSCALL_sleep:
        ctx->cf_r0 = sys_sleep(ctx->cf_r0);
        break;
    case SYSCALL_nanosleep:
        {
            struct timespec *rqtp = (struct timespec *)(ctx->cf_r0);
            struct timespec *rmtp = (struct timespec *)(ctx->cf_r1);
            if(IN_USER_VM(rqtp, sizeof(struct timespec)) &&
               ((rmtp == NULL) || IN_USER_VM(rmtp, sizeof(struct timespec))))
                ctx->cf_r0 = sys_nanosleep(rqtp, rmtp);
			else
	            ctx->cf_r0 = -1;
        }
        break;
    case SYSCALL_gettimeofday:
        {
            struct timeval *tv = ( struct timeval *)(ctx->cf_r0);
            void *tzp = ( void *)(ctx->cf_r1);
            ctx->cf_r0 = sys_gettimeofday(tv, tzp);
        }
        break;
	case SYSCALL_open:
		{
	        char *path = (char *)ctx->cf_r0;
            int mode = ctx->cf_r1;
            if(!IN_USER_VM(path, strlen(path))) {
				ctx->cf_r0 = -1;
                break;
            }

            ctx->cf_r0 = sys_open(path, mode);
            break;
		}
	case SYSCALL_close:
		{
            int fd = ctx->cf_r0;
            ctx->cf_r0 = sys_close(fd);
            break;
		}
	case SYSCALL_read:
		{
            int fd = ctx->cf_r0;
            uint8_t *buffer = (uint8_t *)ctx->cf_r1;
            uint32_t size = ctx->cf_r2;
            if(!IN_USER_VM(buffer, size)) {
				ctx->cf_r0 = -1;
                break;
            }
            ctx->cf_r0 = sys_read(fd, buffer, size);
            break;
		}
	case SYSCALL_write:
		{
            int fd = ctx->cf_r0;
            uint8_t *buffer = (uint8_t *)ctx->cf_r1;
            uint32_t size = ctx->cf_r2;
            if(!IN_USER_VM(buffer, size)) {
				ctx->cf_r0 = -1;
                break;
            }
            ctx->cf_r0 = sys_write(fd, buffer, size);
            break;
		}
	case SYSCALL_lseek:
		{
            int fd = ctx->cf_r0,
                whence = ctx->cf_r2;
            off_t offset = ctx->cf_r1;
            ctx->cf_r0 = sys_seek(fd, offset, whence);
            break;
		}
	case SYSCALL_ioctl:
		{
            int fd = ctx->cf_r0;
            uint32_t cmd = ctx->cf_r1;
            void *arg = (void *)ctx->cf_r2;
            if((arg != NULL) && !IN_USER_VM(arg, 0)) {
				ctx->cf_r0 = -1;
                break;
            }
            ctx->cf_r0 = sys_ioctl(fd, cmd, arg);
            break;
		}

    default:
        printk("syscall #%d not implemented.\r\n", ctx->cf_r4);
		ctx->cf_r0 = -ctx->cf_r4;
        break;
    }
}

/**
 * page fault处理函数。
 * 特别注意：此时系统的中断处于打开状态
 */
int do_page_fault(struct context *ctx, uint32_t vaddr, uint32_t code)
{
    uint32_t i;
    struct vmzone *z;
	char *fmt = "PF:0x%08x(0x%08x)%s";

    if((code & (1<<10))/*FS[4]==1*/) {
        printk(fmt, vaddr, code, "->UNKNOWN ABORT\r\n");
        return -1;
    }

    switch((code & 0xf)/*FS[0:3]*/) {
    case 0x1:/*Alignment fault*/
        printk(fmt, vaddr, code, "->ALIGNMENT FAULT\r\n");
        return -1;
        break;
    case 0x5:/*translation fault (section)*/
        /*
         * A section translation fault occurs if:
         *  • The TLB fetches a first level translation table descriptor,
         *    and this first level descriptor is invalid. This is the case
         *    when bits[1:0] of this descriptor are b00 or b11.
         *            -- ARM1176JZF-S, Revision: r0p7, Technical Reference Manual, 6.9.3
         */
    case 0x7:/*translation fault (page)*/
        break;
    case 0x3:/*access bit fault (section)*/
    case 0x6:/*access bit fault (page)*/
    case 0x9:/*domain fault (section)*/
    case 0xb:/*domain fault (page)*/
    case 0xd:/*permission fault (section)*/
    case 0xf:/*permission fault (page)*/
        printk(fmt, vaddr, code, "->PROTECTION VIOLATION\r\n");
        return -1;
        break;
    default:
        printk(fmt, vaddr, code, "->UNKNOWN ABORT\r\n");
        return -1;
        break;
    }

    /*检查地址是否合法*/
    z = page_zone(vaddr);
    if(z == NULL || z->prot == PROT_NONE) {
        printk(fmt, vaddr, code, "->ILLEGAL MEMORY ACCESS\r\n");
        return -1;
    }
    if(z->flags & MAP_STACK) {
		if(vaddr < (z->base + PAGE_SIZE)) {
			printk(fmt, vaddr, code, "->STACK OVERFLOW\r\n");
			return -1;
		}
    }

    {
        uint32_t paddr, vaddr0=PAGE_TRUNCATE(vaddr);
        uint32_t flags = L2E_V|L2E_C;

        if(z->prot & PROT_WRITE)
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
                    *(PTD+i+((vaddr0-USER_MAX_ADDR))/L2_TABLE_SIZE) = (paddr+i*L2_TABLE_SIZE)|L1E_V;
                }
            }

            *vtopte(vaddr) = paddr|flags;
            invlpg(vaddr);

            if(z->fp != NULL) {
				if(z->fp->fs->seek(z->fp, vaddr0-z->base+z->offset, SEEK_SET) >= 0 &&
				   z->fp->fs->read(z->fp, (void *)(vaddr0), PAGE_SIZE) >= 0)
					;
				else
					;
            } else
				memset((void *)(vaddr0), 0, PAGE_SIZE);

            return 0;
        } else {
            /*物理内存已耗尽*/
            printk(fmt, vaddr, code, "->OUT OF RAM\r\n");
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
     * 页目录放在物理地址[0x4000, 0x8000]
     */
    pgdir=(uint32_t *)(LOADADDR-L1_TABLE_SIZE);
    memset(pgdir, 0, L1_TABLE_SIZE);

    /*
     * 分配4张小页表，并填充到页目录
     * 用于映射页表自身所占的虚拟内存范围，即[0xbfc00000, 0xc0000000]
     */
    uint32_t *ptpte = (uint32_t *)physfree;
    for(i = 0; i < PAGE_SIZE/L2_TABLE_SIZE; i++) {
        pgdir[i+(/*((uint32_t)PT)*/USER_MAX_ADDR>>PGDR_SHIFT)] = (physfree)|L1E_V;
        memset((void *)physfree, 0, L2_TABLE_SIZE);
        physfree+=L2_TABLE_SIZE;
    }

    /*
     * 分配小页表，并填充到页目录，也填充到ptpte
     */
    pte = (uint32_t *)physfree;
    for(i = 0; i < NR_KERN_PAGETABLE; i++) {
        pgdir[i] = pgdir[i+(KERNBASE>>PGDR_SHIFT)] = (physfree)|L1E_V;

        if((i & 3) == 0)
            ptpte[3*L2_ENTRY_COUNT+(i>>2)] = (physfree)|L2E_V|L2E_W|L2E_C;

        memset((void *)physfree, 0, L2_TABLE_SIZE);
        physfree += L2_TABLE_SIZE;
    }
    ptpte[3*L2_ENTRY_COUNT-1] = ((uint32_t)ptpte)|L2E_V|L2E_W|L2E_C;

    /*
     * 设置恒等映射，填充小页表
     * 映射虚拟地址[0, R(_end)]和[KERNBASE, _end]到物理地址为[0, R(_end)]
     */
    for(i = 0; i < (uint32_t)ptpte; i+=PAGE_SIZE)
      pte[i>>PAGE_SHIFT] = i|L2E_V|L2E_W|L2E_C;

    /*
     * 打开分页
     * ARM1176JZF-S TRM, r0p7, p. 3-57,59,60,63,44 and 6-37
     */
    __asm__ __volatile__ (
            "mcr p15,0,%0,c2,c0,0 @TTBR0\n\t"
            "mcr p15,0,%1,c2,c0,2 @TTBCR\n\t"
            "\n\t"
            "mcr p15,0,%2,c3,c0,0 @DACR\n\t"
            "\n\t"
            "mrc p15,0,r0,c1,c0,0 @SCTLR\n\t"
            "orr r0,r0,%3\n\t"
            "mcr p15,0,r0,c1,c0,0\n\t"
            :
            : "r"(pgdir),
              "r"(1<<5),     /* Disable TTBR1 */
              "r"(1),        /* D0=Client, D1-D15=No access */
              "r"(1|(1<<23)) /* SCTLR.{M,XP}=1 */
            : "r0"
    );

    return physfree;
}

/**
 * 初始化物理内存
*/
static void init_ram(uint32_t physfree)
{
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),            // Message size
        MAILBOX_REQUEST,        // Request/Response

        TAG_GET_ARM_MEMORY,     // Tag
        8,                      // # bytes of buffer
        0,                      // Request/Response
        0,                      // buffer
        0,

        0,                      // End Tag
    };

    mailbox_write_read(CHANNEL_TAGS, (uint32_t)&msg[0]);

    g_ram_zone[0] = physfree;
    g_ram_zone[1] = msg[5]+msg[6];
    g_ram_zone[2] = 0;
    g_ram_zone[3] = 0;
}

__attribute__((naked))
static void trampoline()
{
    __asm__ __volatile__("add sp, sp, %0\n\t"
                         "add lr, lr, %0\n\t"
                         "bx  lr\n\t"
                         :
                         : "r" (KERNBASE));
}

/**
 * 机器相关（Machine Dependent）的初始化
 */
static void md_startup(uint32_t mbi, uint32_t physfree)
{
    physfree=init_paging(physfree);

    /*
     * 分页已经打开，切换到虚拟地址运行
     */
	trampoline();

    /*
     * 映射虚拟地址[MMIO_BASE_VA, MMIO_BASE_VA+16M)
     * 到物理地址[MMIO_BASE_PA, MMIO_BASE_PA+16M)
     */
    page_map(MMIO_BASE_VA,
             (cpuid == CPUID_BCM2835)?0x20000000:0x3f000000,
             4096, L2E_V|L2E_W);

    /*
     * 获取电路板信息
     */
    uint32_t  __attribute__((aligned(16))) msg[8] =
    {
        sizeof(msg),            // Message size
        MAILBOX_REQUEST,        // Request/Response

        TAG_GET_BOARD_REVISION, // Tag
        4,                      // # bytes of buffer
        0,                      // Request/Response
        0,                      // buffer

        0,                      // End Tag
    };
    mailbox_write_read(CHANNEL_TAGS, (uint32_t)&msg[0]);
    boardid = msg[5];

    /*
     * 初始化外设
     */
    init_uart(115200);
    init_pic();
    init_pit(HZ);

    /*
     * 初始化物理内存区域
     */
    init_ram(physfree);
}

/**
 * 这个函数是内核的C语言入口，被entry.S调用
 */
void cstart(uint32_t magic, uint32_t mbi)
{
    uint32_t _end = PAGE_ROUNDUP(R((uint32_t)(&end)));



    /*
     * 机器相关（Machine Dependent）的初始化
     */
    md_startup(mbi, _end);

    /*
     * 机器无关（Machine Independent）的初始化
     */
    mi_startup();
}
