#include "utils.h"
#include "kernel.h"
#include "pe.h"

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

/*
    struct segment {
      uint32_t base;
      uint32_t length;
      uint32_t flags;
      
      struct segment *next;
    } *g_user_segment_head = NULL;
*/

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
        printk("PF: Invalid memory access: 0x%08x(0x%08x)\n\r", vaddr, code);
        return -1;
      }
    } else {
      if((vaddr >= (uint32_t)vtopte(USER_MIN_ADDR)) && 
         (vaddr <  (uint32_t)vtopte(USER_MAX_ADDR)))
        code |= PTE_U;

      if ((vaddr >= USER_MIN_ADDR) || 
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
      printk("PF: 0x%08x(0x%08x) -> 0x%08x\n\r", vaddr, code, paddr);
      return 0;
    }
  }
  printk("PF: 0x%08x(0x%08x) ->   ????????\n\r", vaddr, code);
  return -1;
}

void cstart(void)
{
  init_machdep( PAGE_ROUNDUP( R((uint32_t)(&end)) ) );

  g_kern_cur_addr=KERNBASE+PAGE_ROUNDUP( R((uint32_t)(&end)) );
  g_kern_end_addr=KERNBASE+NR_KERN_PAGETABLE*PAGE_SIZE*1024;

//  printk("g_kern_cur_addr=0x%08x, g_kern_end_addr=0x%08x\n\r", g_kern_cur_addr, g_kern_end_addr);

  /**
   *
   *
   */
  if(1){
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

  if(1){
    uint32_t i;
    for(i = 0; i < NR_IRQ; i++)
      intr_vector[i]=isr_default;
  }

  if(1){
    uint32_t size;
    uint32_t i, vaddr, paddr;

    size=((g_mem_zone[1/*XXX*/]-g_mem_zone[0/*XXX*/])>>PAGE_SHIFT);
    size=PAGE_ROUNDUP(size);
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

//    printk("vtopte(0x%08x)=0x%08x\n\r", 0x0, vtopte(0x0));
//    printk("vtopte(0x%08x)=0x%08x\n\r", USER_MAX_ADDR, vtopte(USER_MAX_ADDR));
//    printk("vtopte(0x%08x)=0x%08x\n\r", USER_MIN_ADDR, vtopte(USER_MIN_ADDR));

//    printk("g_frame_freemap=0x%08x\n\r", g_frame_freemap);
//    printk("g_frame_count=%d\n\r", g_frame_count);
//    printk("vtopte(0x%08x)=0x%08x\n\r", g_frame_freemap, vtopte((uint32_t)g_frame_freemap));
//    printk("*vtopte(0x%08x)=0x%08x\n\r", g_frame_freemap, *vtopte((uint32_t)g_frame_freemap));
  }

  if(1){
    g_kern_heap_base = (uint8_t *)g_kern_cur_addr;
    g_kern_heap_size = 1024 * PAGE_SIZE;
    g_kern_cur_addr += g_kern_heap_size;
    init_kmalloc(g_kern_heap_base, g_kern_heap_size);
  }

  intr_vector[IRQ_TIMER] = isr_timer;
  enable_irq(IRQ_TIMER);

  init_task();
  init_callout();

#if 1
  if(1){
    g_task_running = task0;

    __asm__ __volatile__ (
      "movl %0, %%eax\n\t"
      "movl (%%eax), %%eax\n\t"
      "pushl $1f\n\t"
      "popl (52+4)(%%eax)\n\t"
      "movl %%eax, %%esp\n\t"
      "ret\n\t"
      "1:\n\t"
      :
      :"m"(task0)
      :"%eax"
    );
  }

  printk("I'm the task #%d\n\r", task_getid());
#endif

  init_floppy();
  init_fat();

#if 1  
  if(1){
    uint32_t entry;
    int tid;

    char *filename="\\a.exe";

    entry = load_pe(filename);

    printk("entry=0x%08x\n\r", entry);

    if(entry) {
      uint32_t flags;
      save_flags_cli(flags);
      do_page_fault(USER_MAX_ADDR-PAGE_SIZE, PTE_U);
      restore_flags(flags);

      tid = task_create(USER_MAX_ADDR, (void *)entry, (void *)0x19770802);
//      printk("task #%d created\n\r", tid);
//      printk("%d: task #0x%08x created\n\r", task_getid(), tid);
    }
  }
#endif


  while(1) {
    int i;
    for(i = 0; i < 1000000; i++)
      ;
    putchar('K');
  }
}

