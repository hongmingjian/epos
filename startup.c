/**
 *
 * Copyright (C) 2013 Hong MingJian
 * All rights reserved.
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
#include "utils.h"
#include "kernel.h"

void (*intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

uint32_t g_mem_zone[MEM_ZONE_LEN];

uint32_t  *PT  = (uint32_t *)USER_MAX_ADDR,
          *PTD = (uint32_t *)KERN_MIN_ADDR;

uint32_t g_kern_cur_addr;
uint32_t g_kern_end_addr;

uint8_t *g_frame_freemap;
uint32_t g_frame_count = 0;

uint8_t *g_kern_heap_base;
uint32_t g_kern_heap_size = 0;

void isr_default(uint32_t irq, struct context *ctx)
{
  //printk("IRQ=0x%02x\n\r", irq);
}

int do_page_fault(uint32_t vaddr, uint32_t code)
{
  if((code&PTE_V) == 0) {
    int i, found = 0;
    
    if(code&PTE_U) {
      if ((vaddr <  USER_MIN_ADDR) || 
          (vaddr >= USER_MAX_ADDR)) {
        printk("PF: Invalid memory access: 0x%08x(0x%02x)\n\r", vaddr, code);
        return -1;
      }
    } else {
      if((vaddr >= (uint32_t)vtopte(USER_MIN_ADDR)) && 
         (vaddr <  (uint32_t)vtopte(USER_MAX_ADDR)))
        code |= PTE_U;

      if ((vaddr >= USER_MIN_ADDR) && 
          (vaddr <  USER_MAX_ADDR))
        code |= PTE_U;
    }

    for(i = 0; i < g_frame_count; i++) {
      if(g_frame_freemap[i] == 0) {
        found = 1;
        break;
      }
    }

    if(found) {
      uint32_t paddr;
      g_frame_freemap[i] = 1;
      paddr = g_mem_zone[0/*XXX*/]+(i<<PAGE_SHIFT);
      *vtopte(vaddr)=paddr|PTE_V|PTE_RW|(code&PTE_U);
      invlpg(vaddr);
//      printk("PF: 0x%08x(0x%02x) -> 0x%08x\n\r", vaddr, code, paddr);
      return 0;
    }
  }
  printk("PF: 0x%08x(0x%02x) ->   ????????\n\r", vaddr, code);
  return -1;
}

void start_user_task()
{
  char *filename="\\a.exe";
  uint32_t entry;

  printk("From now on, we're running as task #%d\n\r", task_getid());

  init_floppy();
  init_fat();

  entry = load_pe(filename);

  if(entry) {
    int tid;
    tid = task_create(USER_MAX_ADDR, (void *)entry, (void *)0x19770802);
    if(tid < 0)
      printk("failed to create the first user task\n\r");
  } else
    printk("load_pe(%s) failed\n\r", filename);
}

void cstart(uint32_t magic, uint32_t addr)
{
  init_machdep( addr, PAGE_ROUNDUP( R((uint32_t)(&end)) ) );

  printk("Welcome to EPOS\n\r");
  printk("Copyright (C) 2005-2013 MingJian Hong<hmj@cqu.edu.cn>\n\r");
  printk("All rights reserved.\n\r\n\r");
  printk("Partial financial support from the School of Software Engineering,\n\r");
  printk("ChongQing University is gratefully acknowledged.\n\r\n\r");

  g_kern_cur_addr=KERNBASE+PAGE_ROUNDUP( R((uint32_t)(&end)) );
  g_kern_end_addr=KERNBASE+NR_KERN_PAGETABLE*PAGE_SIZE*1024;

//  printk("g_kern_cur_addr=0x%08x, g_kern_end_addr=0x%08x\n\r", g_kern_cur_addr, g_kern_end_addr);

  /**
   *
   *
   */
  if(1) {
    uint32_t i;


    /**
     * XXX - should be elsewhere
     *
     */
    __asm__ __volatile__ (
      "addl %0,%%esp\n\t"
      "addl %0,%%ebp\n\t"
      "pushl $1f\n\t"
      "ret\n\t"
      "1:\n\t"
      :
      :"i"(KERNBASE)
      :
      );

    for(i = 0; i < NR_KERN_PAGETABLE; i++)
      PTD[i] = 0;

    invltlb();
  }

  if(1) {
    uint32_t i;
    for(i = 0; i < NR_IRQ; i++)
      intr_vector[i]=isr_default;
  }

  if(1) {
    uint32_t size;
    uint32_t i, vaddr, paddr;

    size = (g_mem_zone[1/*XXX*/] - g_mem_zone[0/*XXX*/]) >> PAGE_SHIFT;
    size = PAGE_ROUNDUP(size);
    g_frame_freemap = (uint8_t *)g_kern_cur_addr;
    g_kern_cur_addr += size;

    g_mem_zone[1/*XXX*/] -= size;

    vaddr = (uint32_t)g_frame_freemap;
    paddr = g_mem_zone[1];
    for(i =0 ;i < (size>>PAGE_SHIFT); i++) {
        *vtopte(vaddr)=paddr|PTE_V|PTE_RW;
        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }
    memset(g_frame_freemap, 0, size);

    g_frame_count = (g_mem_zone[1]-g_mem_zone[0])>>PAGE_SHIFT;

//    printk("g_frame_freemap=0x%08x\n\r", g_frame_freemap);
//    printk("g_frame_count=%d\n\r", g_frame_count);
  }

  if(1) {
    g_kern_heap_base = (uint8_t *)g_kern_cur_addr;
    g_kern_heap_size = 1024 * PAGE_SIZE;
    g_kern_cur_addr += g_kern_heap_size;
    init_kmalloc(g_kern_heap_base, g_kern_heap_size);
  }

  intr_vector[IRQ_TIMER] = isr_timer;
  enable_irq(IRQ_TIMER);

  init_task();
  init_callout();

  move_to_task0(task0);
  start_user_task();

  while(1)
    hlt();
}

