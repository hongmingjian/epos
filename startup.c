/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
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
#include "utils.h"
#include "kernel.h"
#include "dosfs.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

uint32_t g_ram_zone[RAM_ZONE_LEN];

/*The pointers to the page tables and page directory*/
uint32_t *PT  = (uint32_t *)USER_MAX_ADDR,
         *PTD = (uint32_t *)KERN_MIN_ADDR;

uint32_t g_kern_cur_addr;
uint32_t g_kern_end_addr;

uint8_t *g_frame_freemap;
uint32_t g_frame_count;

uint8_t *g_kern_heap_base;
uint32_t g_kern_heap_size;


#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#define CMOS_READ(addr) ({ \
        outportb(0x70, 0x80|addr); \
        inportb(0x71); \
        })
/*计算机启动时，自1970-01-01 00:00:00 +0000 (UTC)以来的秒数*/
time_t g_startup_time;

VOLINFO g_volinfo;

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    //printk("IRQ=0x%02x\r\n", irq);
}

/*
 * These are the interfaces required by the dosfs
 */
uint32_t DFS_ReadSector(uint8_t unit, uint8_t *buffer,
        uint32_t sector, uint32_t count)
{
    unsigned long i;

    for (i=0;i<count;i++) {
#if USE_FLOPPY
        unsigned char *p;
        if((p=floppy_read_sector(sector)) == NULL) {
            printk("floppy_read_sector failed\r\n");
            return -1;
        }
        memcpy(buffer, p, SECTOR_SIZE);
#else
        ide_read_sector(0x1f0, 0, sector, buffer);
#endif

        sector++;
        buffer += SECTOR_SIZE;
    }

    return 0;
}

uint32_t DFS_WriteSector(uint8_t unit, uint8_t *buffer,
        uint32_t sector, uint32_t count)
{
    unsigned long i;

    for (i=0;i<count;i++) {
#if USE_FLOPPY
        if(floppy_write_sector(sector, buffer) < 0) {
            printk("floppy_write_sector failed\r\n");
            return -1;
        }
#else
        ide_write_sector(0x1f0, 0, sector, buffer);
#endif
        sector++;
        buffer += SECTOR_SIZE;
    }

    return 0;
}

/*
 * 这个函数被线程task0执行。它首先初始化软盘/硬盘，
 * 然后加载应用程序a.out，并创建了一个用户线程执行a.out中的代码
 */
void start_user_task()
{
    char *filename="a.out";
    uint32_t entry;

#if USE_FLOPPY
    printk("task #%d: Initializing floppy disk controller...",
            sys_task_getid());
    init_floppy();
    printk("Done\r\n");
#else
    printk("task #%d: Initializing IDE controller...", sys_task_getid());
    ide_init(0x1f0);
    printk("Done\r\n");
#endif

    printk("task #%d: Initializing PCI controller...", sys_task_getid());
    pci_init();
    printk("Done\r\n");

    {
        uint32_t pstart;
        uint8_t scratch[SECTOR_SIZE];

        printk("task #%d: Initializing FAT file system...", sys_task_getid());

#if USE_FLOPPY
        pstart = 0;
#else
        pstart = DFS_GetPtnStart(0, scratch, 0, NULL, NULL, NULL);
        if (pstart == 0xffffffff) {
            printk("Failed\r\n");
            return;
        }
#endif

        if(DFS_GetVolInfo(0, scratch, pstart, &g_volinfo)) {
            printk("Failed\r\n");
            return;
        }
        printk("Done\r\n");
    }

    printk("task #%d: Loading %s...", sys_task_getid(), filename);
    entry = load_pe(&g_volinfo, filename);

    if(entry) {
        printk("Done\r\n");

        {
            int i;
            for(i = 1; i < 5/*XXX*/; i++)
                *vtopte(PAGE_TRUNCATE(entry)+i*PAGE_SIZE*1024) = 0;
        }

        printk("task #%d: Creating first user task...", sys_task_getid());
        if(sys_task_create((void *)USER_MAX_ADDR, (void *)entry, (void *)0x12345678) == NULL)
            printk("Failed\r\n");
        else {
            printk("Done\r\n");
        }

    } else
        printk("Failed\r\n");
}

/*
 * 这个函数是内核的C语言入口，被entry.S调用
 */
void cstart(uint32_t magic, uint32_t mbi)
{
    init_machdep( mbi, PAGE_ROUNDUP( R((uint32_t)(&end)) ) );

    printk("Welcome to EPOS\r\n");
    printk("Copyright (C) 2005-2013 MingJian Hong<hongmingjian@gmail.com>\r\n");
    printk("All rights reserved.\r\n\r\n");

    g_kern_cur_addr=KERNBASE+PAGE_ROUNDUP( R((uint32_t)(&end)) );
    g_kern_end_addr=KERNBASE+NR_KERN_PAGETABLE*PAGE_SIZE*1024;

    //printk("g_kern_cur_addr=0x%08x, g_kern_end_addr=0x%08x\r\n",
    //       g_kern_cur_addr, g_kern_end_addr);

    if(1) {
        uint32_t i;


        /**
         * XXX - machine-dependent should be elsewhere
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
         * The kernel has been relocated to the linked address. The identity
         * mapping is unmapped.
         */
        for(i = 1; i < NR_KERN_PAGETABLE; i++)
            PTD[i] = 0;

        /*清空TLB*/
        invltlb();
    }

    /*
     * Reserve the address space for the freemap, which is used
     * to manage the free physical memory.
     *
     * The freemap is then mapped to the top of the physical memory.
     */
    if(1) {
        uint32_t size;
        uint32_t i, vaddr, paddr;

        size = (g_ram_zone[1/*XXX*/] - g_ram_zone[0/*XXX*/]) >> PAGE_SHIFT;
        size = PAGE_ROUNDUP(size);
        g_frame_freemap = (uint8_t *)g_kern_cur_addr;
        g_kern_cur_addr += size;

        g_ram_zone[1/*XXX*/] -= size;

        vaddr = (uint32_t)g_frame_freemap;
        paddr = g_ram_zone[1];
        for(i =0 ;i < (size>>PAGE_SHIFT); i++) {
            *vtopte(vaddr)=paddr|PTE_V|PTE_W;
            vaddr += PAGE_SIZE;
            paddr += PAGE_SIZE;
        }
        memset(g_frame_freemap, 0, size);

        g_frame_count = (g_ram_zone[1]-g_ram_zone[0])>>PAGE_SHIFT;

        printk("Available memory: 0x%08x - 0x%08x (%d pages)\r\n\r\n",
                g_ram_zone[0], g_ram_zone[1], g_frame_count);

        //printk("g_frame_freemap=0x%08x\r\n", g_frame_freemap);
        //printk("g_frame_count=%d\r\n", g_frame_count);
    }

    /*
     * 初始化内核堆，为使用kmalloc/kfree做准备.
     */
    if(1) {
        g_kern_heap_base = (uint8_t *)g_kern_cur_addr;
        g_kern_heap_size = 1024 * PAGE_SIZE;
        g_kern_cur_addr += g_kern_heap_size;
        init_kmalloc(g_kern_heap_base, g_kern_heap_size);
    }

    /*
     * 保存计算机启动的时间，即自1970-01-01 00:00:00 +0000 (UTC)以来的秒数
     */
    if(1) {
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
        if((time.tm_year+1900) < 1970)
            time.tm_year += 100;

        g_startup_time = mktime(&time);
    }

    /*
     * 初始化中断向量表
     */
    if(1) {

        /*安装默认的中断处理程序*/
        uint32_t i;
        for(i = 0; i < NR_IRQ; i++)
            g_intr_vector[i]=isr_default;

        /*安装定时器的中断处理程序*/
        g_intr_vector[IRQ_TIMER] = isr_timer;
        enable_irq(IRQ_TIMER);

        /*安装键盘的中断处理程序*/
        g_intr_vector[IRQ_KEYBOARD] = isr_keyboard;
        enable_irq(IRQ_KEYBOARD);
    }

    /*
     * 初始化多线程子系统
     */
    init_task();

    /*
     * task0是系统空闲线程，已经由init_task创建。这里用run_as_task0手工切换到task0运行。
     * 由task0初始化其他子系统，并启动第一个用户线程，然后它将循环执行函数cpu_idle。
     */
    run_as_task0();
    start_user_task();
    while(1)
        cpu_idle();
}

