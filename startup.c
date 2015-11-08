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
    uint32_t entry, end;

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

	if(1) {

		struct ETH_HEADER {
			uint8_t rmac[6];
			uint8_t smac[6];
			uint16_t type;
		} __attribute__((packed));

		struct ARP_PACKET {
			struct ETH_HEADER eth;
			uint16_t hw_type;
			uint16_t proto_type;
			uint8_t hw_len;
			uint8_t proto_len;
			uint16_t op;
			uint8_t smac[6];
			uint8_t sip[4];
			uint8_t dmac[6];
			uint8_t dip[4];
		} __attribute__((packed));

		uint8_t maddr[6];
		e1000_init();

		struct ARP_PACKET arp;
		int i;

		memset(&arp, 0, sizeof(arp));

		for (i = 0; i < 6; i++){
			arp.eth.rmac[i] = 0xff;
		}

		e1000_getmac(&arp.eth.smac[0]);
		arp.eth.type = htons(0x0806);
		arp.hw_type = htons(1);
		arp.proto_type = htons(0x0800);
		arp.hw_len = 6;
		arp.proto_len = 4;
		arp.op = htons(1);
		e1000_getmac(&arp.smac[0]);

		arp.sip[0] = 0xc0;//192
		arp.sip[1] = 0xa8;//168
		arp.sip[2] = 0x1;//1
		arp.sip[3] = 0x22;//34

		arp.dip[0] = 0xc0;//192
		arp.dip[1] = 0xa8;//168
		arp.dip[2] = 0x1;//1
		arp.dip[3] = 0x1;//1

		e1000_send((uint8_t *)&arp, sizeof(arp));
	}

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

/*
 * 这个函数是内核的C语言入口，被entry.S调用
 */
void cstart(uint32_t magic, uint32_t mbi)
{
    init_machdep( mbi, PAGE_ROUNDUP( R((uint32_t)(&end)) ) );

    printk("Welcome to EPOS\r\n");
    printk("Copyright (C) 2005-2013 MingJian Hong<hongmingjian@gmail.com>\r\n");
    printk("All rights reserved.\r\n\r\n");

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
     * 初始化地址空间
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

