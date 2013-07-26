#include "utils.h"
#include "kernel.h"

void (*intr_vector[NR_IRQ])(struct frame fr);

uint32_t g_mem_zone[MEM_ZONE_LEN];

uint32_t  *PT  = (uint32_t *)((((KERNBASE>>22)-1)<<22)),
          *PTD = (uint32_t *)((((KERNBASE>>22)-1)<<22)+(((KERNBASE>>22)-1)<<PAGE_SHIFT));

uint32_t g_kern_cur_addr;
uint32_t g_kern_end_addr;

uint8_t *g_frame_freemap;
uint32_t g_frame_count = 0;

uint8_t *g_kern_heap_base;
uint32_t g_kern_heap_size = 0;

void isr_default(struct frame fr)
{
  //printk("IRQ=0x%02x\n\r", fr.trapno);
}

int do_page_fault(uint32_t vaddr, uint32_t code)
{
//  printk("page fault: 0x%08x, 0x%08x\n\r", vaddr, code);
  if((code & 1) == 0) {
    int i, found = 0;
    for(i = 0; i < g_frame_count; i++) {
      if(g_frame_freemap[i] == 0) {
        found = 1;
        break;
      }
    }

    if(found) {
      uint32_t paddr;
      paddr = g_mem_zone[0/*XXX*/]+(i<<PAGE_SHIFT);
      g_frame_freemap[i] = 1;
      *vtopte(vaddr)=paddr|PTE_V|PTE_RW;
      invlpg(vaddr);
//      printk("0x%08x -> 0x%08x\n\r", vaddr, paddr);
      return 0;
    }
  }
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

  sti();

  while(1)
    ;
}

