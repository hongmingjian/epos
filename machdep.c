#include "machdep.h"
#include "kernel.h"


/**
 *
 *
 *
 */
#define	IO_ICU1		0x020		/* 8259A Interrupt Controller #1 */
#define	IO_ICU2		0x0A0		/* 8259A Interrupt Controller #2 */
#define	IRQ_SLAVE	0x04
#define	ICU_SLAVEID	2
#define ICU_IMR_OFFSET		1	/* IO_ICU{1,2} + 1 */
static void init_i8259(uint8_t idt_offset)
{
  outportb(IO_ICU1, 0x11);//ICW1
	outportb(IO_ICU1+ICU_IMR_OFFSET, idt_offset);	//ICW2
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

/**
 *
 *
 */
static void init_i8253(uint32_t hz)
{
  uint16_t latch = 1193182/hz;
  outportb(0x43, 0x36);
  outportb(0x40, latch&0xff);
  outportb(0x40, (latch&0xff00)>>16);
}

void enable_irq(uint32_t irq)
{
  if(irq < 8)
    outportb(IO_ICU1+ICU_IMR_OFFSET, inportb(IO_ICU1+ICU_IMR_OFFSET) & (~(1<<irq)));
  else if(irq < NR_IRQ) {
    irq -= 8;
    outportb(IO_ICU2+ICU_IMR_OFFSET, inportb(IO_ICU2+ICU_IMR_OFFSET) & (~(1<<irq)));
  }
}

void disable_irq(uint32_t irq)
{
  if(irq < 8)
    outportb(IO_ICU1+ICU_IMR_OFFSET, inportb(IO_ICU1+ICU_IMR_OFFSET) | (1<<irq));
  else if(irq < NR_IRQ) {
    irq -= 8;
    outportb(IO_ICU2+ICU_IMR_OFFSET, inportb(IO_ICU1+ICU_IMR_OFFSET) | (1<<irq));
  }
}

/**
 *
 *
 *
 */
void switch_to(struct tcb *new)
{
    if(g_task_running != NULL) {
      __asm__(
        "pushal\n\t"
        "movl %0, %%eax\n\t"
        "popl 0(%%eax)\n\t"
        "popl 4(%%eax)\n\t"
        "popl 8(%%eax)\n\t"
        "popl 12(%%eax)\n\t"
        "popl 16(%%eax)\n\t"
        "popl 20(%%eax)\n\t"
        "popl 24(%%eax)\n\t"
        "popl 28(%%eax)\n\t"
        "leal 1f, %%ecx\n\t"
        "movl %%ecx, 40(%%eax)\n\t"
        "pushl %%cs\n\t"
        "popl 44(%%eax)\n\t"
        "pushfl\n\t"
        "popl 48(%%eax)\n\t"
        ::"m"(g_task_running)
        );
    }

  g_task_running = new;

  __asm__(
    "movl %0, %%eax\n\t"
    "movl 12(%%eax), %%esp\n\t"
    "pushl 48(%%eax)\n\t"
    "pushl 44(%%eax)\n\t"
    "pushl 40(%%eax)\n\t"
    "pushl 28(%%eax)\n\t"
    "pushl 24(%%eax)\n\t"
    "pushl 20(%%eax)\n\t"
    "pushl 16(%%eax)\n\t"
    "pushl $0\n\t"
    "pushl 8(%%eax)\n\t"
    "pushl 4(%%eax)\n\t"
    "pushl 0(%%eax)\n\t"
    "popal\n\t"
    "iret\n\t"
    "1:\n\t"
    ::"m"(g_task_running)
    );
}


/**
 *
 *
 *
 */
#define	SEL_KPL	0		/* kernel priority level */
#define	SEL_UPL	3		/* user priority level */

#define	GSEL_NULL   0	/* Null Descriptor */
#define	GSEL_KCODE  1	/* Kernel Code Descriptor */
#define	GSEL_KDATA  2	/* Kernel Data Descriptor */
#define	GSEL_UCODE  3	/* User Code Descriptor */
#define	GSEL_UDATAL	4	/* Kernel Data Descriptor */
#define	GSEL_TSS    5	/* TSS Descriptor */
#define NR_GDT        6

static
struct segment_descriptor	{
	unsigned lolimit:16 ;
	unsigned lobase:24 __attribute__ ((packed));
	unsigned type:5 ;
	unsigned dpl:2 ;
	unsigned p:1 ;
	unsigned hilimit:4 ;
	unsigned xx:2 ;
	unsigned def32:1 ;
	unsigned gran:1 ;
	unsigned hibase:8 ;
} gdt[NR_GDT] = {
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
   uint32_t prev;
   uint32_t esp0;       // loaded when we change to kernel mode.
   uint32_t ss0;        // ditto
   uint32_t esp1;       // everything below here is unusued 
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
   uint16_t trap;
   uint16_t iomap_base;
} __attribute__ ((packed)) tss;

struct region_descriptor {
	unsigned limit:16;	
	unsigned base:32 __attribute__ ((packed));
};

void lgdt(struct region_descriptor *rdp);
static void init_gdt(void)
{
    struct region_descriptor rd;
    uint32_t base = (uint32_t)&tss, limit = base + sizeof(struct tss);

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
    tss.ss0  = GSEL_KDATA;
    tss.esp0 = 0;/*XXX*/

    __asm__ __volatile__(
        "movw $((8*5)|3), %%ax\n\t"
        "ltr %%ax\n\t"
        :
        :
        :"%ax"
        );
}


/**
 *
 *
 *
 */
typedef void (*idt_handler_t)(uint32_t eip, uint32_t cs, uint32_t eflags, uint32_t esp, uint32_t ss);
#define	IDT_EXCEPTION(name)	__CONCAT(exception_,name)
extern idt_handler_t
  IDT_EXCEPTION(divide_error),    IDT_EXCEPTION(debug),             IDT_EXCEPTION(nmi),          
  IDT_EXCEPTION(breakpoint),      IDT_EXCEPTION(overflow),          IDT_EXCEPTION(bounds_check),      
  IDT_EXCEPTION(inval_opcode),    IDT_EXCEPTION(double_fault),      IDT_EXCEPTION(copr_not_avail), 
  IDT_EXCEPTION(copr_seg_overrun),IDT_EXCEPTION(inval_tss),         IDT_EXCEPTION(segment_not_present), 
  IDT_EXCEPTION(stack_exception), IDT_EXCEPTION(general_protection),IDT_EXCEPTION(page_fault),   
  IDT_EXCEPTION(intel_reserved),  IDT_EXCEPTION(copr_error),        IDT_EXCEPTION(alignment_check),   
  IDT_EXCEPTION(machine_check),   IDT_EXCEPTION(simd_fp_exception); 

#define	IDT_INTERRUPT(name)	__CONCAT(hwint,name)
extern idt_handler_t
  IDT_INTERRUPT(00),IDT_INTERRUPT(01), IDT_INTERRUPT(02), IDT_INTERRUPT(03),
  IDT_INTERRUPT(04),IDT_INTERRUPT(05), IDT_INTERRUPT(06), IDT_INTERRUPT(07),
  IDT_INTERRUPT(08),IDT_INTERRUPT(09), IDT_INTERRUPT(10), IDT_INTERRUPT(11),
  IDT_INTERRUPT(12),IDT_INTERRUPT(13), IDT_INTERRUPT(14), IDT_INTERRUPT(15);

#define ICU_IDT_OFFSET 32
#define NR_IDT (ICU_IDT_OFFSET+NR_IRQ)
static
struct gate_descriptor	{
	unsigned looffset:16 ;
	unsigned selector:16 ;
	unsigned stkcpy:5 ;
	unsigned xx:3 ;
	unsigned type:5 ;
  #define	GT_386INTR	14	/* 386 interrupt gate */
  #define	GT_386TRAP	15	/* 386 trap gate */

	unsigned dpl:2 ;
	unsigned p:1 ;
	unsigned hioffset:16 ;
} idt[NR_IDT];

static void setidt(int idx, idt_handler_t *func, int typ)
{
	struct gate_descriptor *ip;

	ip = idt + idx;
	ip->looffset = (uint32_t)func;
	ip->selector = (GSEL_KCODE << 3)|SEL_KPL;
	ip->stkcpy = 0;
	ip->xx = 0;
	ip->type = typ;
	ip->dpl = SEL_KPL;
	ip->p = 1;
	ip->hioffset = ((uint32_t)func)>>16 ;
}

void lidt(struct region_descriptor *rdp);

static void init_idt()
{  
    int i;
    struct region_descriptor rd;

    for (i = 0; i < NR_IDT; i++)
      setidt(i, &IDT_EXCEPTION(intel_reserved), GT_386TRAP);

  	setidt(0,  &IDT_EXCEPTION(divide_error),         GT_386TRAP);
  	setidt(1,  &IDT_EXCEPTION(debug),                GT_386TRAP);
  	setidt(2,  &IDT_EXCEPTION(nmi),                  GT_386TRAP);
    setidt(3,  &IDT_EXCEPTION(breakpoint),           GT_386TRAP);
    setidt(4,  &IDT_EXCEPTION(overflow),             GT_386TRAP);
    setidt(5,  &IDT_EXCEPTION(bounds_check),         GT_386TRAP);
    setidt(6,  &IDT_EXCEPTION(inval_opcode),         GT_386TRAP);
    setidt(7,  &IDT_EXCEPTION(copr_not_avail),       GT_386TRAP);
    setidt(8,  &IDT_EXCEPTION(double_fault),         GT_386TRAP);
    setidt(9,  &IDT_EXCEPTION(copr_seg_overrun),     GT_386TRAP);
    setidt(10, &IDT_EXCEPTION(inval_tss),            GT_386TRAP);
    setidt(11, &IDT_EXCEPTION(segment_not_present),  GT_386TRAP);
    setidt(12, &IDT_EXCEPTION(stack_exception),      GT_386TRAP);
    setidt(13, &IDT_EXCEPTION(general_protection),   GT_386TRAP);
    setidt(14, &IDT_EXCEPTION(page_fault),           GT_386INTR/*Ref. III-5.12.1.2*/);
    setidt(15, &IDT_EXCEPTION(intel_reserved),       GT_386TRAP);
    setidt(16, &IDT_EXCEPTION(copr_error),           GT_386TRAP);
    setidt(17, &IDT_EXCEPTION(alignment_check),      GT_386TRAP);
    setidt(18, &IDT_EXCEPTION(machine_check),        GT_386TRAP);
    setidt(19, &IDT_EXCEPTION(simd_fp_exception),    GT_386TRAP);
    
    setidt(ICU_IDT_OFFSET+0, &IDT_INTERRUPT(00), GT_386INTR);
    setidt(ICU_IDT_OFFSET+1, &IDT_INTERRUPT(01), GT_386INTR);
    setidt(ICU_IDT_OFFSET+2, &IDT_INTERRUPT(02), GT_386INTR);
    setidt(ICU_IDT_OFFSET+3, &IDT_INTERRUPT(03), GT_386INTR);
    setidt(ICU_IDT_OFFSET+4, &IDT_INTERRUPT(04), GT_386INTR);
    setidt(ICU_IDT_OFFSET+5, &IDT_INTERRUPT(05), GT_386INTR);
    setidt(ICU_IDT_OFFSET+6, &IDT_INTERRUPT(06), GT_386INTR);
    setidt(ICU_IDT_OFFSET+7, &IDT_INTERRUPT(07), GT_386INTR);
    setidt(ICU_IDT_OFFSET+8, &IDT_INTERRUPT(08), GT_386INTR);
    setidt(ICU_IDT_OFFSET+9, &IDT_INTERRUPT(09), GT_386INTR);
    setidt(ICU_IDT_OFFSET+10,&IDT_INTERRUPT(10), GT_386INTR);
    setidt(ICU_IDT_OFFSET+11,&IDT_INTERRUPT(11), GT_386INTR);
    setidt(ICU_IDT_OFFSET+12,&IDT_INTERRUPT(12), GT_386INTR);
    setidt(ICU_IDT_OFFSET+13,&IDT_INTERRUPT(13), GT_386INTR);
    setidt(ICU_IDT_OFFSET+14,&IDT_INTERRUPT(14), GT_386INTR);
    setidt(ICU_IDT_OFFSET+15,&IDT_INTERRUPT(15), GT_386INTR);

    rd.limit = NR_IDT*sizeof(idt[0]) - 1;
    rd.base = (uint32_t) idt;
    lidt(&rd);
}

/**
 *
 *
 *
 */
void exception(struct frame fr)
{
  switch(fr.trapno) {
  case 14://page fault
    {
      uint32_t vaddr;
      __asm__ __volatile__("movl %%cr2,%0" : "=r" (vaddr));
      if(do_page_fault(vaddr, fr.code) == 0)
        return;
    }
    break;
  }
  printk("edi=0x%08x, esi=0x%08x\n\r", fr.edi, fr.esi);
  printk("ebp=0x%08x, isp=0x%08x\n\r", fr.ebp, fr.isp);
  printk("ebx=0x%08x, edx=0x%08x\n\r", fr.ebx, fr.edx);
  printk("ecx=0x%08x, eax=0x%08x\n\r", fr.ecx, fr.eax);
  printk("trapno=0x%02x, code=0x%08x\n\r", fr.trapno, fr.code);
  printk("eip=0x%08x, cs=0x%04x, eflags=0x%08x\n\r", fr.eip, fr.cs, fr.eflags);
  printk("esp=0x%08x\n\r", fr.esp);
  printk("ss=0x%04x\n\r", fr.ss);
  while(1);
}

/**
 *
 *
 */
int putchar(int c)
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

/**
 *
 *
 *
 */
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

    pgdir[(KERNBASE>>PGDR_SHIFT)-1]=(uint32_t)(pgdir)|PTE_V|PTE_RW;

    pte=(uint32_t *)(PAGE_TRUNCATE(pgdir[0]));
    for(i = 0; i < (uint32_t)(pgdir); i+=PAGE_SIZE) {
      pte[i>>PAGE_SHIFT]=(i)|PTE_V|PTE_RW;
    }

    __asm__ __volatile__ (
  	  "movl	%0, %%eax\n\t"
  	  "movl	%%eax, %%cr3\n\t"
  	  "movl	%%cr0, %%eax\n\t"
  	  "orl	$0x80000000, %%eax\n\t"
  	  "movl	%%eax, %%cr0\n\t"
//      "pushl $1f\n\t"
//      "ret\n\t"
//      "1:\n\t"
      :
      :"m"(pgdir)
      :"%eax"
    );

    return physfree;
}

/**
 *
 *
 *
 */
static void init_mem(uint32_t physfree)
{
    uint32_t i, n = 0;

    struct SMAP_entry {
  	  uint32_t BaseL;
  	  uint32_t BaseH;
  	  uint32_t LengthL; 
  	  uint32_t LengthH;
  	  uint32_t Type;
      #define SMAP_TYPE_RAM		1 /**< Normal memory */
//      #define SMAP_TYPE_RESERVED	2 /**< Reserved and unavailable */
//      #define SMAP_TYPE_ACPI		3 /**< ACPI reclaim memory */
//      #define SMAP_TYPE_NVS		4 /**< ACPI NVS memory */
    
  	  uint32_t ACPI;
    }__attribute__((packed)) *smap=(struct SMAP_entry *)0x804/*XXX*/;

    for(i = 0; i < *((uint32_t*)0x800/*XXX*/); i++) {
      if(smap[i].Type==SMAP_TYPE_RAM) {
        g_mem_zone[n]=PAGE_TRUNCATE(smap[i].BaseL);
        g_mem_zone[n+1]=PAGE_TRUNCATE(g_mem_zone[n]+smap[i].LengthL);

        if(g_mem_zone[n+1]<g_mem_zone[n]+256*PAGE_SIZE)
          continue;

        if((physfree > g_mem_zone[n]) && 
           (physfree <= g_mem_zone[n+1]))
          g_mem_zone[n]=physfree;

        if(g_mem_zone[n+1] >= g_mem_zone[n] + PAGE_SIZE) {
//          printk("Memory: 0x%08x-0x%08x\n\r", g_mem_zone[n], g_mem_zone[n+1]);
          n+=2;
          if(n + 2 >= MEM_ZONE_LEN)
            break;
        }
      }
    }

    g_mem_zone[n] = 0;
    g_mem_zone[n+1] = 0;
  }

void init_machdep(uint32_t physfree)
{
  physfree=init_paging(physfree);

  init_gdt();
  init_idt();

  init_mem(physfree);

  init_i8259(ICU_IDT_OFFSET);
  init_i8253(HZ);
}
