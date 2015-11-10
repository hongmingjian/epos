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
#include "utils.h"
#include "kernel.h"
#include "dosfs.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*可用的物理内存区域*/
uint32_t g_ram_zone[RAM_ZONE_LEN];

uint32_t *PT  = (uint32_t *)USER_MAX_ADDR, //页表的指针
         *PTD = (uint32_t *)KERN_MIN_ADDR; //页目录的指针

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#define CMOS_READ(addr) ({ \
        outportb(0x70, 0x80|addr); \
        inportb(0x71); \
        })
/*计算机启动时，自1970-01-01 00:00:00 +0000 (UTC)以来的秒数*/
time_t g_startup_time;

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    //printk("IRQ=0x%02x\r\n", irq);
}

/**
 * These are the interfaces required by the dosfs
 */
VOLINFO g_volinfo;
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

/**
 * 这个函数被线程task0执行。它加载应用程序a.out，
 * 并创建了一个用户线程执行a.out中的main函数
 */
void start_user_task()
{
    char *filename="a.out";
    uint32_t entry, end;

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
    entry = load_pe(&g_volinfo, filename, &end);

    if(entry) {
        printk("Done\r\n");

        printk("task #%d: Creating first user task...", sys_task_getid());

        /* XXX - 为第一个用户线程准备一个堆，大小64MiB */
        page_alloc_in_addr(end, 64*1024*1024/PAGE_SIZE);

        /* XXX - 为第一个用户线程准备栈，大小1MiB */
        page_alloc_in_addr(USER_MAX_ADDR - (1024*1024), (1024*1024)/PAGE_SIZE);
        if(sys_task_create((void *)USER_MAX_ADDR, (void *)entry, (void *)0x12345678) == NULL)
            printk("Failed\r\n");
        else {
            printk("Done\r\n");
        }

    } else
        printk("Failed\r\n");
}

/**
 * 机器无关（Machine Independent）的初始化
 */
void mi_startup()
{
    printk("Welcome to EPOS\r\n");
    printk("Copyright (C) 2005-2015 MingJian Hong<hongmingjian@gmail.com>\r\n");
    printk("All rights reserved.\r\n\r\n");

    /*
     * 初始化虚拟地址空间
     */
    init_page();

    /*
     * 初始化物理内存管理器
     */
    init_frame();

    /*
     * 初始化内核堆，大小为4MiB，由kmalloc/kfree管理.
     */
    init_kmalloc((uint8_t *)page_alloc(1024, 0), 1024 * PAGE_SIZE);

#if USE_FLOPPY
    printk("Initializing floppy disk controller...");
    init_floppy();        //初始化软盘控制器
    printk("Done\r\n");
#else
    printk("Initializing IDE controller...");
    ide_init(0x1f0);      //初始化IDE控制器
    printk("Done\r\n");
#endif
    printk("Initializing PCI controller...");
    pci_init();           //初始化PCI总线控制器
    printk("Done\r\n");

    e1000_init();         //初始化E1000网卡

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
     * task0是系统空闲线程，已经由init_task创建。
     * 这里用run_as_task0手工切换到task0运行。
     * 由task0启动第一个用户线程，然后它将循环执行函数cpu_idle。
     */
    run_as_task0();
    start_user_task();
    while(1)
        cpu_idle();
}

