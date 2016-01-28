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

#include "kernel.h"
#include "multiboot.h"

#define __CONCAT1(x,y)  x ## y
#define __CONCAT(x,y)   __CONCAT1(x,y)
#define __STRING(x)     #x              /* stringify without expanding x */
#define __XSTRING(x)    __STRING(x)     /* expand x, then stringify */

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#define CMOS_READ(addr) ({ \
        outportb(0x70, 0x80|addr); \
        inportb(0x71); \
        })
/*计算机启动时，自1970-01-01 00:00:00 +0000 (UTC)以来的秒数*/
time_t g_startup_time;

#define IO_ICU1  0x20  /* 8259A Interrupt Controller #1 */
#define IO_ICU2  0xA0  /* 8259A Interrupt Controller #2 */
#define IRQ_SLAVE 0x04
#define ICU_SLAVEID 2
#define ICU_IMR_OFFSET  1 /* IO_ICU{1,2} + 1 */
/**
 * 初始化i8259中断控制器
 */
static void init_i8259(uint8_t idt_offset)
{
    outportb(IO_ICU1, 0x11);//ICW1
    outportb(IO_ICU1+ICU_IMR_OFFSET, idt_offset); //ICW2
    outportb(IO_ICU1+ICU_IMR_OFFSET, IRQ_SLAVE); //ICW3
    outportb(IO_ICU1+ICU_IMR_OFFSET, 1); //ICW4
    outportb(IO_ICU1+ICU_IMR_OFFSET, 0xff); //OCW1
    outportb(IO_ICU1, 0x0a); //OCW3

    outportb(IO_ICU2, 0x11); //ICW1
    outportb(IO_ICU2+ICU_IMR_OFFSET, idt_offset+8); //ICW2
    outportb(IO_ICU2+ICU_IMR_OFFSET, ICU_SLAVEID); //ICW3
    outportb(IO_ICU2+ICU_IMR_OFFSET,1); //ICW4
    outportb(IO_ICU2+ICU_IMR_OFFSET, 0xff); //OCW1
    outportb(IO_ICU2, 0x0a); //OCW3

    //打开ICU2中断
    outportb(IO_ICU1+ICU_IMR_OFFSET,
             inportb(IO_ICU1+ICU_IMR_OFFSET) & (~(1<<2)));
}

/**
 * 初始化i8253定时器
 */
static void init_i8253(uint32_t freq)
{
    uint16_t latch = 1193182/freq;
    outportb(0x43, 0x36);
    outportb(0x40, latch&0xff);
    outportb(0x40, (latch&0xff00)>>16);
}

/**
 * 让中断控制器打开某个中断
 */
void enable_irq(uint32_t irq)
{
    uint8_t val;
    if(irq < 8){
        val = inportb(IO_ICU1+ICU_IMR_OFFSET);
        outportb(IO_ICU1+ICU_IMR_OFFSET, val & (~(1<<irq)));
    } else if(irq < NR_IRQ) {
        irq -= 8;
        val = inportb(IO_ICU2+ICU_IMR_OFFSET);
        outportb(IO_ICU2+ICU_IMR_OFFSET, val & (~(1<<irq)));
    }
}

/**
 * 让中断控制器关闭某个中断
 */
void disable_irq(uint32_t irq)
{
    uint8_t val;
    if(irq < 8) {
        val = inportb(IO_ICU1+ICU_IMR_OFFSET);
        outportb(IO_ICU1+ICU_IMR_OFFSET, val | (1<<irq));
    } else if(irq < NR_IRQ) {
        irq -= 8;
        val = inportb(IO_ICU2+ICU_IMR_OFFSET);
        outportb(IO_ICU2+ICU_IMR_OFFSET, val | (1<<irq));
    }
}

/**
 * 把CPU从当前线程切换去运行线程new，即上下文切换（Context switch）
 */
void switch_to(struct tcb *new)
{
    __asm__ __volatile__ (
            "pushal\n\t"
            "pushl $1f\n\t"
            "movl %0, %%eax\n\t"
            "movl %%esp, (%%eax)\n\t"
            "addl $36, %%esp\n\t"
            :
            :"m"(g_task_running)
            :"%eax"
            );

    g_task_running = new;

    __asm__ __volatile__ (
            "movl %0, %%eax\n\t"
            "movl (%%eax), %%esp\n\t"
            "ret\n\t"
            "1:\n\t"
            "movl %%cr0, %%eax\n\t"
            "orl $8, %%eax\n\t"
            "movl %%eax, %%cr0\n\t"
            "popal\n\t"
            :
            :"m"(g_task_running)
            :"%eax"
            );
}

static
struct segment_descriptor gdt[NR_GDT] = {
    {// GSEL_NULL
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0 },
    { // GSEL_KCODE
        0xffff,
        0x0,
        0x1a,
        SEL_KPL,
        0x1,
        0xf,
        0x0,
        0x1,
        0x1,
        0x0 },
    { // GSEL_KDATA
        0xffff,
        0x0,
        0x12,
        SEL_KPL,
        0x1,
        0xf,
        0x0,
        0x1,
        0x1,
        0x0 },
    { // GSEL_UCODE
        0xffff,
        0x0,
        0x1a,
        SEL_UPL,
        0x1,
        0xf,
        0x0,
        0x1,
        0x1,
        0x0 },
    { // GSEL_UDATA
        0xffff,
        0x0,
        0x12,
        SEL_UPL,
        0x1,
        0xf,
        0x0,
        0x1,
        0x1,
        0x0 },
    { // GSEL_TSS, to be filled
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0,
        0x0 }
};

static
struct tss {
    uint32_t prev; // UNUSED
    uint32_t esp0; // loaded when CPU changed from user to kernel mode.
    uint32_t ss0;  // ditto
    uint32_t esp1; // everything below is UNUSUED
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t debug;
    uint16_t iomap_base;
} __attribute__ ((packed)) tss;

struct region_descriptor {
    unsigned limit:16;
    unsigned base:32 __attribute__ ((packed));
};

extern char tmp_stack;

void lgdt(struct region_descriptor *rdp);
static void init_gdt(void)
{
    struct region_descriptor rd;
    uint32_t base = (uint32_t)&tss, limit = sizeof(struct tss);

    gdt[GSEL_TSS].lolimit = limit & 0xffff;
    gdt[GSEL_TSS].lobase = base & 0xffffff;
    gdt[GSEL_TSS].type = 9;
    gdt[GSEL_TSS].dpl = SEL_UPL;
    gdt[GSEL_TSS].p = 1;
    gdt[GSEL_TSS].hilimit = (limit&0xf0000)>>16;
    gdt[GSEL_TSS].xx = 0;
    gdt[GSEL_TSS].def32 = 0;
    gdt[GSEL_TSS].gran = 0;
    gdt[GSEL_TSS].hibase = (base&0xff000000)>>24;

    rd.limit = NR_GDT*sizeof(gdt[0]) - 1;
    rd.base =  (uint32_t) gdt;
    lgdt(&rd);

    memset(&tss, 0, sizeof(struct tss));
    tss.ss0  = GSEL_KDATA*sizeof(gdt[0]);
    tss.esp0 = (uint32_t)&tmp_stack;

    __asm__ __volatile__(
            "movw %0, %%ax\n\t"
            "ltr %%ax\n\t"
            :
            :"i"((GSEL_TSS * sizeof(gdt[0])) | SEL_UPL)
            :"%ax"
            );
}

typedef void (*idt_handler_t)(uint32_t eip, uint32_t cs, uint32_t eflags,
        uint32_t esp, uint32_t ss);
#define IDT_EXCEPTION(name) __CONCAT(exception_,name)
extern idt_handler_t
    IDT_EXCEPTION(divide_error),    IDT_EXCEPTION(debug),
    IDT_EXCEPTION(nmi),             IDT_EXCEPTION(breakpoint),
    IDT_EXCEPTION(overflow),        IDT_EXCEPTION(bounds_check),
    IDT_EXCEPTION(inval_opcode),    IDT_EXCEPTION(double_fault),
    IDT_EXCEPTION(copr_not_avail),  IDT_EXCEPTION(copr_seg_overrun),
    IDT_EXCEPTION(inval_tss),       IDT_EXCEPTION(segment_not_present),
    IDT_EXCEPTION(stack_fault),     IDT_EXCEPTION(general_protection),
    IDT_EXCEPTION(page_fault),      IDT_EXCEPTION(intel_reserved),
    IDT_EXCEPTION(copr_error),      IDT_EXCEPTION(alignment_check),
    IDT_EXCEPTION(machine_check),   IDT_EXCEPTION(simd_fp),
    int0x82_syscall;

#define IDT_INTERRUPT(name) __CONCAT(hwint,name)
extern idt_handler_t
    IDT_INTERRUPT(00),IDT_INTERRUPT(01), IDT_INTERRUPT(02), IDT_INTERRUPT(03),
    IDT_INTERRUPT(04),IDT_INTERRUPT(05), IDT_INTERRUPT(06), IDT_INTERRUPT(07),
    IDT_INTERRUPT(08),IDT_INTERRUPT(09), IDT_INTERRUPT(10), IDT_INTERRUPT(11),
    IDT_INTERRUPT(12),IDT_INTERRUPT(13), IDT_INTERRUPT(14), IDT_INTERRUPT(15);

/**
 * `struct gate_descriptor' comes from FreeBSD
 */
#define ICU_IDT_OFFSET 32
#define NR_IDT 131
static
struct gate_descriptor {
    unsigned looffset:16 ;
    unsigned selector:16 ;
    unsigned stkcpy:5 ;
    unsigned xx:3 ;
    unsigned type:5 ;
#define GT_386INTR 14 /* 386 interrupt gate */
#define GT_386TRAP 15 /* 386 trap gate */

    unsigned dpl:2 ;
    unsigned p:1 ;
    unsigned hioffset:16 ;
} idt[NR_IDT];

/**
 * `setidt' comes from FreeBSD
 */
static void setidt(int idx, idt_handler_t *func, int typ, int dpl)
{
    struct gate_descriptor *ip;

    ip = idt + idx;
    ip->looffset = (uint32_t)func;
    ip->selector = (GSEL_KCODE * sizeof(gdt[0])) | SEL_KPL;
    ip->stkcpy = 0;
    ip->xx = 0;
    ip->type = typ;
    ip->dpl = dpl;
    ip->p = 1;
    ip->hioffset = ((uint32_t)func)>>16 ;
}

void lidt(struct region_descriptor *rdp);

static void init_idt()
{
    int i;
    struct region_descriptor rd;

    for (i = 0; i < NR_IDT; i++)
        setidt(i, &IDT_EXCEPTION(intel_reserved), GT_386TRAP, SEL_KPL);

    setidt(0,  &IDT_EXCEPTION(divide_error),        GT_386INTR, SEL_KPL);
    setidt(1,  &IDT_EXCEPTION(debug),               GT_386INTR, SEL_KPL);
    setidt(2,  &IDT_EXCEPTION(nmi),                 GT_386INTR, SEL_KPL);
    setidt(3,  &IDT_EXCEPTION(breakpoint),          GT_386INTR, SEL_KPL);
    setidt(4,  &IDT_EXCEPTION(overflow),            GT_386INTR, SEL_KPL);
    setidt(5,  &IDT_EXCEPTION(bounds_check),        GT_386INTR, SEL_KPL);
    setidt(6,  &IDT_EXCEPTION(inval_opcode),        GT_386INTR, SEL_KPL);
    setidt(7,  &IDT_EXCEPTION(copr_not_avail),      GT_386INTR, SEL_KPL);
    setidt(8,  &IDT_EXCEPTION(double_fault),        GT_386INTR, SEL_KPL);
    setidt(9,  &IDT_EXCEPTION(copr_seg_overrun),    GT_386INTR, SEL_KPL);
    setidt(10, &IDT_EXCEPTION(inval_tss),           GT_386INTR, SEL_KPL);
    setidt(11, &IDT_EXCEPTION(segment_not_present), GT_386INTR, SEL_KPL);
    setidt(12, &IDT_EXCEPTION(stack_fault),         GT_386INTR, SEL_KPL);
    setidt(13, &IDT_EXCEPTION(general_protection),  GT_386INTR, SEL_KPL);
    setidt(14, &IDT_EXCEPTION(page_fault),          GT_386INTR, SEL_KPL);
    setidt(15, &IDT_EXCEPTION(intel_reserved),      GT_386INTR, SEL_KPL);
    setidt(16, &IDT_EXCEPTION(copr_error),          GT_386INTR, SEL_KPL);
    setidt(17, &IDT_EXCEPTION(alignment_check),     GT_386INTR, SEL_KPL);
    setidt(18, &IDT_EXCEPTION(machine_check),       GT_386INTR, SEL_KPL);
    setidt(19, &IDT_EXCEPTION(simd_fp),             GT_386INTR, SEL_KPL);

    setidt(ICU_IDT_OFFSET+0, &IDT_INTERRUPT(00), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+1, &IDT_INTERRUPT(01), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+2, &IDT_INTERRUPT(02), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+3, &IDT_INTERRUPT(03), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+4, &IDT_INTERRUPT(04), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+5, &IDT_INTERRUPT(05), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+6, &IDT_INTERRUPT(06), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+7, &IDT_INTERRUPT(07), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+8, &IDT_INTERRUPT(08), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+9, &IDT_INTERRUPT(09), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+10,&IDT_INTERRUPT(10), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+11,&IDT_INTERRUPT(11), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+12,&IDT_INTERRUPT(12), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+13,&IDT_INTERRUPT(13), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+14,&IDT_INTERRUPT(14), GT_386INTR, SEL_KPL);
    setidt(ICU_IDT_OFFSET+15,&IDT_INTERRUPT(15), GT_386INTR, SEL_KPL);

    setidt(0x82, &int0x82_syscall, GT_386INTR, SEL_UPL/*!*/);

    rd.limit = NR_IDT*sizeof(idt[0]) - 1;
    rd.base = (uint32_t) idt;
    lidt(&rd);
}

/**
 * 系统调用putchar的执行函数
 *
 * 往屏幕上的当前光标位置打印一个字符，相应地移动光标的位置
 */
int sys_putchar(int c)
{
    unsigned char *SCREEN_BASE = (char *)(0xB8000+KERNBASE);
    unsigned int curpos, i;

    uint32_t flags;

    /*
     * 读取当前光标位置
     */
    save_flags_cli(flags);
    outportb(0x3d4, 0x0e);
    curpos = inportb(0x3d5);
    curpos <<= 8;
    outportb(0x3d4, 0x0f);
    curpos += inportb(0x3d5);
    curpos <<= 1;

    switch(c) {
    case '\n'://换行，只是换行而已
        curpos = (curpos/160)*160 + 160;
        break;
    case '\r'://回车，只是回车而已
        curpos = (curpos/160)*160;
        break;
    case '\t':
        curpos += 8;
        break;
    case '\b':
        curpos -= 2;
        SCREEN_BASE[curpos] = 0x20;
        break;
    default:
        SCREEN_BASE[curpos++] = c;
        SCREEN_BASE[curpos++] = 0x07;
        break;
    }

    /*
     * 滚动屏幕
     */
    if(curpos >= 160*25) {
        for(i = 0; i < 160*24; i++) {
            SCREEN_BASE[i] = SCREEN_BASE[i+160];
        }
        for(i = 0; i < 80; i++) {
            SCREEN_BASE[(160*24)+(i*2)  ] = 0x20;
            SCREEN_BASE[(160*24)+(i*2)+1] = 0x07;
        }
        curpos -= 160;
    }

    /*
     * 保存当前光标位置
     */
    curpos >>= 1;
    outportb(0x3d4, 0x0f);
    outportb(0x3d5, curpos & 0x0ff);
    outportb(0x3d4, 0x0e);
    outportb(0x3d5, curpos >> 8);
    restore_flags(flags);

    return c;
}

/**
 * 系统调用beep的执行函数
 * 让蜂鸣器以频率freq发声，如果freq=0表示关闭蜂鸣器
 */
void sys_beep(int freq)
{
    if(freq <= 0)
        outportb (0x61, 0);
    else {
        uint32_t flags;

        freq = 1193182 / freq;

        save_flags_cli(flags);
        outportb (0x43, 0xB6);
        outportb (0x42,  freq       & 0xFF);
        outportb (0x42, (freq >> 8) & 0xFF);
        restore_flags(flags);

        outportb (0x61, 3);
    }
}

/**
 * CPU异常处理程序，ctx保存了发生异常时CPU各个寄存器的值
 */
int exception(struct context *ctx)
{
    switch(ctx->exception) {
    case 14://page fault
        {
            uint32_t vaddr;
            int res;
            __asm__ __volatile__("movl %%cr2,%0" : "=r" (vaddr));
            sti();
            res = do_page_fault(ctx, vaddr, ctx->errorcode);
            cli();
            if(res == 0)
                return 0;
        }
        break;

    case 13://general protection
        if(ctx->eflags & 0x20000) {
            if(vm86_emulate((struct vm86_context *)ctx))
                //This exception was eaten by vm86mon and return to vm86 mode
                return 0;
            else {
                //This exception cannot be eaten by vm86mon, return to user mode
                struct context *c=(struct context *)(((uint8_t *)g_task_running)-
                                                     sizeof(struct context));
                **((struct vm86_context **)(c->esp+4)) = *((struct vm86_context *)ctx);
                return 1;
            }
        }
        break;

    case 7://device not available
        __asm__ __volatile__("clts\t\n");

        if(g_task_own_fpu == g_task_running)
            return 0;

        __asm__ __volatile__("fwait\t\n");

        if(g_task_own_fpu) {
            __asm__ __volatile__("fnsave %0\t\n"::"m"(g_task_own_fpu->fpu));
        }

        g_task_own_fpu = g_task_running;

        __asm__ __volatile__("frstor %0\t\n"::"m"(g_task_running->fpu));

        return 0;

        break;

    case 16://x87 FPU Floating-Point Error
        if(g_task_own_fpu) {
            __asm__ __volatile__("fnsave %0\t\n"::"m"(g_task_own_fpu->fpu));
            printk("fpu.cwd=0x%04x\r\n", g_task_own_fpu->fpu.cwd);
            printk("fpu.swd=0x%04x\r\n", g_task_own_fpu->fpu.swd);
            printk("fpu.twd=0x%04x\r\n", g_task_own_fpu->fpu.twd);
            printk("fpu.fip=0x%08x\r\n", g_task_own_fpu->fpu.fip);
            printk("fpu.fcs=0x%04x\r\n", g_task_own_fpu->fpu.fcs);
            printk("fpu.foo=0x%08x\r\n", g_task_own_fpu->fpu.foo);
            printk("fpu.fos=0x%04x\r\n", g_task_own_fpu->fpu.fos);
        }
    }

    printk("Un-handled exception!\r\n");
    printk(" fs=0x%08x,  es=0x%08x,  ds=0x%08x\r\n",
            ctx->fs, ctx->es, ctx->ds);
    printk("edi=0x%08x, esi=0x%08x, ebp=0x%08x, isp=0x%08x\r\n",
            ctx->edi, ctx->esi, ctx->ebp, ctx->isp);
    printk("ebx=0x%08x, edx=0x%08x, ecx=0x%08x, eax=0x%08x\r\n",
            ctx->ebx, ctx->edx, ctx->ecx, ctx->eax);
    printk("exception=0x%02x, errorcode=0x%08x\r\n",
            ctx->exception, ctx->errorcode);
    printk("eip=0x%08x,  cs=0x%04x, eflags=0x%08x\r\n",
            ctx->eip, ctx->cs, ctx->eflags);
    if(ctx->cs & SEL_UPL)
        printk("esp=0x%08x,  ss=0x%04x\r\n", ctx->esp, ctx->ss);

    while(1);
}

/**
 * 系统调用分发函数，ctx保存了进入内核前CPU各个寄存器的值
 */
void syscall(struct context *ctx)
{
    //printk("task #%d syscalling #%d.\r\n", sys_task_getid(), ctx->eax);
    switch(ctx->eax) {
    case SYSCALL_task_exit:
        sys_task_exit(*((int *)(ctx->esp+4)));
        break;
    case SYSCALL_task_create:
        {
            uint32_t user_stack = *((uint32_t *)(ctx->esp+4));
            uint32_t user_entry = *((uint32_t *)(ctx->esp+8));
            uint32_t user_pvoid = *((uint32_t *)(ctx->esp+12));
            struct tcb *tsk;

            //printk("stack: 0x%08x, entry: 0x%08x, pvoid: 0x%08x\r\n",
            //       user_stack, user_entry, user_pvoid);
            if(!IN_USER_VM(user_stack, 0) ||
               !IN_USER_VM(user_entry, 0)) {
                ctx->eax = -ctx->eax;
                break;
            }
            tsk = sys_task_create((void *)user_stack,
                                  (void (*)(void *))user_entry,
                                  (void *)user_pvoid);
            ctx->eax = (tsk==NULL)?-1:tsk->tid;
        }
        break;
    case SYSCALL_task_getid:
        ctx->eax=sys_task_getid();
        break;
    case SYSCALL_task_yield:
        sys_task_yield();
        break;
    case SYSCALL_task_wait:
        {
            int tid    = *(int  *)(ctx->esp+4);
            int *pcode = *( int **)(ctx->esp+8);
            if((pcode != NULL) && !IN_USER_VM(pcode, sizeof(int))) {
                ctx->eax = -ctx->eax;
                break;
            }

            ctx->eax = sys_task_wait(tid, pcode);
        }
        break;
    case SYSCALL_reboot:
        {
            int howto = *(int *)(ctx->esp+4);
            while(inportb(0x64) & 2)
                ;
            outportb(0x64, 0xFE);
            ctx->eax = -1;
        }
        break;
    case SYSCALL_mmap:
        {
            void *addr = *(void **)(ctx->esp+4);
            size_t len = *(size_t *)(ctx->esp+8);
            int prot = *(int *)(ctx->esp+12);
            int flags = *(int *)(ctx->esp+16);
            int fd = *(int *)(ctx->esp+20);
            off_t offset = *(off_t *)(ctx->esp+24);
            uint32_t npages = PAGE_ROUNDUP(len)/PAGE_SIZE;
            uint32_t va = (uint32_t)addr;

            ctx->eax = -1;
            if(len == 0)
                break;
            if(fd == -1) {
                 if(!(flags & MAP_ANON))
                     break;
            } else {
                if(flags & MAP_ANON)
                    break;
            }
            if(!(flags & MAP_PRIVATE))
                break;

            /*XXX - 0x8000留给/dev/mem*/
            if((fd == 0x8000) && (offset & PAGE_MASK))
                break;

            if(flags & MAP_FIXED) {
                if(!IN_USER_VM(va, len) ||
                   (va & PAGE_MASK)) {
                    break;
                }
                ctx->eax = page_alloc_in_addr(va, npages, prot);
            } else {
                ctx->eax = page_alloc(npages, prot, 1);
            }

            if(ctx->eax != -1 && fd == 0x8000) {
                page_map(ctx->eax, offset,
                         npages, PTE_U|((prot&PROT_WRITE)?PTE_W:0)|PTE_V);
            }
        }
        break;
    case SYSCALL_munmap:
        {
            void *addr = *(void **)(ctx->esp+4);
            size_t len = *(size_t *)(ctx->esp+8);
            uint32_t npages = PAGE_ROUNDUP(len)/PAGE_SIZE;
            uint32_t va = (uint32_t)addr;

            ctx->eax = -1;
            if(len == 0)
                break;

            if(!IN_USER_VM(va, len)) {
                break;
            }

            ctx->eax = page_free(va, npages);

            if(ctx->eax != -1) {
                uint32_t i, x;
                for(i = 0; i < npages; i++) {
                    x = *vtopte(va);
                    if(x & PTE_V) {
                        *vtopte(va) = 0;
                        invlpg(va);

                        //XXX - 可能不是RAM，不能用frame_free
                        frame_free(PAGE_TRUNCATE(x), 1);
                    }
                    va += PAGE_SIZE;
                }
            }
        }
        break;
    case SYSCALL_sleep:
        ctx->eax = sys_sleep((*((int *)(ctx->esp+4))));
        break;
    case SYSCALL_nanosleep:
        {
            ctx->eax = -1;
            struct timespec *rqtp = *(struct timespec **)(ctx->esp+4);
            struct timespec *rmtp = *(struct timespec **)(ctx->esp+8);
            if(IN_USER_VM(rqtp, sizeof(struct timespec)) &&
               ((rmtp == NULL) || IN_USER_VM(rmtp, sizeof(struct timespec))))
                ctx->eax = sys_nanosleep(rqtp, rmtp);
        }
        break;
    case SYSCALL_beep:
        sys_beep((*((int *)(ctx->esp+4))));
        break;
    case SYSCALL_vm86:
        {
            struct vm86_context *p = *(struct vm86_context **)(ctx->esp+4);
            p->eflags &= 0x0ffff;
            p->eflags |= 0x20000;
            sys_vm86(p);
        }
        break;
    case SYSCALL_putchar:
        ctx->eax = sys_putchar((*((int *)(ctx->esp+4)))&0xff);
        break;
    case SYSCALL_getchar:
        ctx->eax = sys_getchar();
        break;
    case SYSCALL_recv:
        {
            int sockfd = *( int *)(ctx->esp+4);
            void *buf = *( uint8_t **)(ctx->esp+8);
            size_t len = *( size_t *)(ctx->esp+12);
            int flags = *( int *)(ctx->esp+16);
            ctx->eax = sys_recv(sockfd, buf, len, flags);
        }
        break;
    case SYSCALL_send:
        {
            int sockfd = *( int *)(ctx->esp+4);
            void *buf = *( uint8_t **)(ctx->esp+8);
            size_t len = *( size_t *)(ctx->esp+12);
            int flags = *( int *)(ctx->esp+16);
            e1000_send(buf, len);
            ctx->eax = len;
        }
        break;
    case SYSCALL_ioctl:
        {
            ctx->eax = -1;
            int fd = *( int *)(ctx->esp+4);
            uint32_t req = *( uint32_t *)(ctx->esp+8);
            uint8_t *pv = *( uint8_t **)(ctx->esp+12);
            if(req == SIOCGIFHWADDR) {
                e1000_getmac(pv);
                ctx->eax = 6;
            }
        }
        break;
    default:
        printk("syscall #%d not implemented.\r\n", ctx->eax);
        ctx->eax = -ctx->eax;
        break;
    }
}

/**
 * 初始化分页子系统
 */
static uint32_t init_paging(uint32_t physfree)
{
    uint32_t i, x, y;
    uint32_t *pgdir, *pte;

    /*
     * 分配页目录
     */
    pgdir=(uint32_t *)physfree;
    physfree += PAGE_SIZE;
    memset(pgdir, 0, PAGE_SIZE);

    /*
     * 分配小页表，并填充页目录
     */
    for(i = 0; i < NR_KERN_PAGETABLE; i++) {
        pgdir[i                       ]=
        pgdir[i+(KERNBASE>>PGDR_SHIFT)]=physfree|PTE_V|PTE_W;
        memset((void *)physfree, 0, PAGE_SIZE);
        physfree+=PAGE_SIZE;
    }

    /*
     * 映射映射虚拟地址[0, R(&end)]和[KERNBASE, &end]到物理地址[0, R(&end)]
     */
    pte=(uint32_t *)(PAGE_TRUNCATE(pgdir[0]));
    for(i = 0; i < (uint32_t)(pgdir); i+=PAGE_SIZE)
        pte[i>>PAGE_SHIFT]=(i)|PTE_V|PTE_W;

    /*
     * 映射页目录及页表
     */
    pgdir[(KERNBASE>>PGDR_SHIFT)-1]=(uint32_t)(pgdir)|PTE_V|PTE_W;

    /*
     * 打开分页
     */
    __asm__ __volatile__ (
            "movl %0, %%eax\n\t"
            "movl %%eax, %%cr3\n\t"
            "movl %%cr0, %%eax\n\t"
            "orl  $0x80000000, %%eax\n\t"
            "movl %%eax, %%cr0\n\t"
            :
            :"m"(pgdir)
            :"%eax"
            );

    return physfree;
}

/**
 * 初始化物理内存
 */
static void init_ram(multiboot_memory_map_t *mmap,
        uint32_t size,
        uint32_t physfree)
{
    int n = 0;

    for (; size;
            size -= (mmap->size+sizeof(mmap->size)),
            mmap = (multiboot_memory_map_t *) ((uint32_t)mmap +
                                               mmap->size +
                                               sizeof (mmap->size))) {
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            g_ram_zone[n  ] = PAGE_TRUNCATE(mmap->addr&0xffffffff);
            g_ram_zone[n+1] = PAGE_TRUNCATE(g_ram_zone[n]+(mmap->len&0xffffffff));

            /*扣除内核所占的物理内存*/
            if((physfree >  g_ram_zone[n  ]) &&
               (physfree <= g_ram_zone[n+1]))
                g_ram_zone[n]=physfree;

            /*为8086模式保留0-4KiB的物理内存*/
            if((PAGE_SIZE >  g_ram_zone[n  ]) &&
               (PAGE_SIZE <= g_ram_zone[n+1]))
                g_ram_zone[n]=PAGE_SIZE;

            if(g_ram_zone[n+1] >= g_ram_zone[n] + PAGE_SIZE) {
                n += 2;
                if(n + 2 >= RAM_ZONE_LEN)
                    break;
            }
        }
    }

    g_ram_zone[n  ] = 0;
    g_ram_zone[n+1] = 0;
}

/**
 * 机器相关（Machine Dependent）的初始化
 */
static void md_startup(uint32_t mbi, uint32_t physfree)
{
    physfree=init_paging(physfree);

    init_gdt();
    init_idt();

    init_ram((void *)(((multiboot_info_t *)mbi)->mmap_addr),
             ((multiboot_info_t *)mbi)->mmap_length,
             physfree);

    init_i8259(ICU_IDT_OFFSET);
    init_i8253(HZ);
}

/**
 * 这个函数是内核的C语言入口，被entry.S调用
 */
void cstart(uint32_t magic, uint32_t mbi)
{
    uint32_t i, _end = PAGE_ROUNDUP(R((uint32_t)(&end)));

    /*
     * 机器相关（Machine Dependent）的初始化
     */
    md_startup(mbi, _end);

    /*
     * 分页已经打开，切换到虚拟地址运行
     */
    __asm__ __volatile__ (
            "addl %0,%%esp\n\t"
            "addl %0,%%ebp\n\t"
            "pushl $1f\n\t"
            "ret\n\t"
            "1:\n\t"
            :
            :"i"(KERNBASE)
            );

    /*
     * 内核已经被重定位到链接地址，取消恒等映射
     */
    for(i = 0; i < NR_KERN_PAGETABLE; i++)
        PTD[i] = 0;

    /*
     * 取消了1MiB以内、除文本显存以外的虚拟内存映射
     */
    for(i = 0; i < 0x100000; i+=PAGE_SIZE)
        if(i != 0xB8000)
            *vtopte(i+KERNBASE)=0;

    /*
     * 初始化数学协处理器
     */
    __asm__ __volatile__ (
            "movl %%cr0, %%eax\n\t"
            "orl  $0x20, %%eax\n\t"
            "movl %%eax, %%cr0\n\t"
            "fninit\n\t"
            :
            :
            :"%eax"
            );

    /*
     * 从CMOS读取计算机启动的时间，即自1970-01-01 00:00:00 +0000 (UTC)以来的秒数
     */
    {
        struct tm time;

        do {
            time.tm_sec  = CMOS_READ(0);
            time.tm_min  = CMOS_READ(2);
            time.tm_hour = CMOS_READ(4);
            time.tm_mday = CMOS_READ(7);
            time.tm_mon  = CMOS_READ(8);
            time.tm_year = CMOS_READ(9);
        } while (time.tm_sec != CMOS_READ(0));
        BCD_TO_BIN(time.tm_sec);
        BCD_TO_BIN(time.tm_min);
        BCD_TO_BIN(time.tm_hour);
        BCD_TO_BIN(time.tm_mday);
        BCD_TO_BIN(time.tm_mon);
        BCD_TO_BIN(time.tm_year);

        time.tm_mon--;
        if((time.tm_year + 1900) < 1970)
            time.tm_year += 100;

        g_startup_time = mktime(&time);
    }

    /*
     * 机器无关（Machine Independent）的初始化
     */
    mi_startup();
}
