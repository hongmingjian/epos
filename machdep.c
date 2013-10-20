/**
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
#include "machdep.h"
#include "kernel.h"
#include "syscall.h"
#include "multiboot.h"

#define IO_ICU1  0x20  /* 8259A Interrupt Controller #1 */
#define IO_ICU2  0xA0  /* 8259A Interrupt Controller #2 */
#define IRQ_SLAVE 0x04
#define ICU_SLAVEID 2
#define ICU_IMR_OFFSET  1 /* IO_ICU{1,2} + 1 */
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
}

static void init_i8253(uint32_t freq)
{
  uint16_t latch = 1193182/freq;
  outportb(0x43, 0x36);
  outportb(0x40, latch&0xff);
  outportb(0x40, (latch&0xff00)>>16);
}

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

void exception(struct context *ctx)
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
        return;
    }
    break;
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

void syscall(struct context *ctx)
{
//  printk("task #%d syscalling #%d.\r\n", sys_task_getid(), ctx->eax);
  switch(ctx->eax) {
  case SYSCALL_TASK_EXIT:
    sys_task_exit(*((uint32_t *)(ctx->esp+4)));
    break;
  case SYSCALL_TASK_CREATE:
    {
      uint32_t user_stack = *((uint32_t *)(ctx->esp+4));
      uint32_t user_entry = *((uint32_t *)(ctx->esp+8));
      uint32_t user_pvoid = *((uint32_t *)(ctx->esp+12));
//      printk("stack: 0x%08x, entry: 0x%08x, pvoid: 0x%08x\r\n",
//             user_stack, user_entry, user_pvoid);
      if((user_stack < USER_MIN_ADDR) || (user_stack >= USER_MAX_ADDR) ||
         (user_entry < USER_MIN_ADDR) || (user_entry >= USER_MAX_ADDR)) {
        ctx->eax = -ctx->eax;
        break;
      }
      ctx->eax = sys_task_create(user_stack,
                                 (void *)user_entry,
                                 (void *)user_pvoid);
    }
    break;
  case SYSCALL_TASK_GETID:
    ctx->eax=sys_task_getid();
    break;
  case SYSCALL_TASK_YIELD:
    sys_task_yield();
    break;
  case SYSCALL_TASK_WAIT:
    {
      uint32_t tid  = *((uint32_t  *)(ctx->esp+4));
       int32_t **code = ( int32_t **)(ctx->esp+8);
      if(((*code) != NULL) &&
         (((uint32_t)(*code) <  USER_MIN_ADDR) ||
          ((uint32_t)(*code) >= USER_MAX_ADDR))) {
        ctx->eax = -ctx->eax;
        break;
      }

      ctx->eax = sys_task_wait(tid, *code);
    }
    break;
  case SYSCALL_BEEP:
    sys_beep((*((uint32_t *)(ctx->esp+4))));
    break;
  case SYSCALL_PUTCHAR:
    ctx->eax = sys_putchar((*((uint32_t *)(ctx->esp+4)))&0xff);
    break;
  default:
    printk("syscall #%d not implemented.\r\n", ctx->eax);
    ctx->eax = -ctx->eax;
    break;
  }
}

int sys_putchar(int c)
{
  unsigned char *SCREEN_BASE = (char *)(KERNBASE+0xB8000);
  unsigned int curpos, i;

  uint32_t flags;

  save_flags_cli(flags);

  outportb(0x3d4, 0x0e);
  curpos = inportb(0x3d5);
  curpos <<= 8;
  outportb(0x3d4, 0x0f);
  curpos += inportb(0x3d5);
  curpos <<= 1;

  switch(c) {
  case '\n':
    curpos = (curpos/160)*160 + 160;
    break;
  case '\r':
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

  curpos >>= 1;
  outportb(0x3d4, 0x0f);
  outportb(0x3d5, curpos & 0x0ff);
  outportb(0x3d4, 0x0e);
  outportb(0x3d5, curpos >> 8);

  restore_flags(flags);

  return c;
}

void sys_beep(uint32_t freq)
{
  freq = freq & 0xffff;

  if(!freq)
    outportb (0x61, inportb(0x61) & 0xFC);
  else {
    freq = 1193182 / freq;
    outportb (0x61, inportb(0x61) | 3);
    outportb (0x43, 0xB6);
    outportb (0x42,  freq       & 0xFF);
    outportb (0x42, (freq >> 8) & 0xFF);
  }
}

static uint32_t init_paging(uint32_t physfree)
{
  uint32_t i;
  uint32_t *pgdir, *pte;

  pgdir=(uint32_t *)physfree;
  physfree += PAGE_SIZE;
  memset(pgdir, 0, PAGE_SIZE);

  for(i = 0; i < NR_KERN_PAGETABLE; i++) {
    pgdir[i]=
    pgdir[i+(KERNBASE>>PGDR_SHIFT)]=physfree|PTE_V|PTE_RW;
    memset((void *)physfree, 0, PAGE_SIZE);
    physfree+=PAGE_SIZE;
  }

  pte=(uint32_t *)(PAGE_TRUNCATE(pgdir[0]));
  for(i = 0; i < (uint32_t)(pgdir); i+=PAGE_SIZE)
    pte[i>>PAGE_SHIFT]=(i)|PTE_V|PTE_RW;

  pgdir[(KERNBASE>>PGDR_SHIFT)-1]=(uint32_t)(pgdir)|PTE_V|PTE_RW;

  __asm__ __volatile__ (
    "movl %0, %%eax\n\t"
    "movl %%eax, %%cr3\n\t"
    "movl %%cr0, %%eax\n\t"
    "orl  $0x80000000, %%eax\n\t"
    "movl %%eax, %%cr0\n\t"
//    "pushl $1f\n\t"
//    "ret\n\t"
//    "1:\n\t"
    :
    :"m"(pgdir)
    :"%eax"
  );

  return physfree;
}

static void init_mem(multiboot_memory_map_t *mmap,
                     uint32_t size,
                     uint32_t physfree)
{
  uint32_t i, n = 0;

  for (; size;
       size -= (mmap->size+sizeof(mmap->size)),
       mmap = (multiboot_memory_map_t *) ((uint32_t)mmap +
                                          mmap->size +
                                          sizeof (mmap->size))) {
    if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
      g_mem_zone[n  ] = PAGE_TRUNCATE(mmap->addr&0xffffffff);
      g_mem_zone[n+1] = PAGE_TRUNCATE(g_mem_zone[n]+(mmap->len&0xffffffff));

      if(g_mem_zone[n+1] < g_mem_zone[n] + 256 * PAGE_SIZE)
        continue;

      if((physfree >  g_mem_zone[n  ]) &&
         (physfree <= g_mem_zone[n+1]))
        g_mem_zone[n]=physfree;

      if(g_mem_zone[n+1] >= g_mem_zone[n] + PAGE_SIZE) {
//        printk("Memory: 0x%08x-0x%08x\r\n", g_mem_zone[n], g_mem_zone[n+1]);
        n += 2;
        if(n + 2 >= MEM_ZONE_LEN)
          break;
      }
    }
  }

  g_mem_zone[n  ] = 0;
  g_mem_zone[n+1] = 0;
}

void init_machdep(uint32_t mbi, uint32_t physfree)
{
  physfree=init_paging(physfree);

  init_gdt();
  init_idt();

  init_mem((void *)(((multiboot_info_t *)mbi)->mmap_addr),
           ((multiboot_info_t *)mbi)->mmap_length,
           physfree);

  init_i8259(ICU_IDT_OFFSET);
  init_i8253(HZ);
}
