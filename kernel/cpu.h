/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2005, 2008, 2013, 2020 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _CPU_H
#define _CPU_H

#define SYS_CLOCK_FREQ 250000000UL

#define CPUID_BCM2835  0x410FB767
#define CPUID_BCM2836  0x410FC073
#define CPUID_BCM2837  0x410FD034
#define CPUID_BCM2711  0x410FD083
#if RPI_QEMU == 1
#define CPUID_QEMU     0x410fc075
#endif

#ifndef __ASSEMBLY__
#include <stdint.h>

extern uint32_t cpuid;

/* 
 * XXX On Pi3 (and all other Pis with recent firmware) 
 *     UART clock is now 48MHz 
 */
#define FUARTCLK        3000000

#define ARMTIMER_REG (0xB400)
typedef struct {
	volatile uint32_t Load;
	volatile uint32_t Value;
	volatile uint32_t Control;
#define ARMTIMER_CTRL_23BIT            (1 << 1)
#define ARMTIMER_CTRL_PRESCALE_1       (0 << 2)
#define ARMTIMER_CTRL_PRESCALE_16      (1 << 2)
#define ARMTIMER_CTRL_PRESCALE_256     (2 << 2)
#define ARMTIMER_CTRL_INTR_ENABLE      (1 << 5)
#define ARMTIMER_CTRL_ENABLE           (1 << 7)

	volatile uint32_t IRQClear;
	volatile uint32_t RAWIRQ;
	volatile uint32_t MaskedIRQ;
	volatile uint32_t Reload;
	volatile uint32_t PreDivider;
	volatile uint32_t FreeRunningCounter;
} armtimer_reg_t;

#define INTR_REG (0xB200)
typedef struct {
	volatile uint32_t IRQ_basic_pending;
	volatile uint32_t IRQ_pending_1;
	volatile uint32_t IRQ_pending_2;
	volatile uint32_t FIQ_control;
	volatile uint32_t Enable_IRQs_1;
	volatile uint32_t Enable_IRQs_2;
	volatile uint32_t Enable_basic_IRQs;
	volatile uint32_t Disable_IRQs_1;
	volatile uint32_t Disable_IRQs_2;
	volatile uint32_t Disable_basic_IRQs;
} intr_reg_t;

#define AUX_REG (0x215000)
typedef struct {
	volatile uint32_t IRQ;
	volatile uint32_t enables;
	volatile uint32_t UNUSED1[14];
	volatile uint32_t mu_io;
	volatile uint32_t mu_ier;
	volatile uint32_t mu_iir;
	volatile uint32_t mu_lcr;
	volatile uint32_t mu_mcr;
	volatile uint32_t mu_lsr;
	volatile uint32_t mu_msr;
	volatile uint32_t mu_scratch;
	volatile uint32_t mu_cntl;
	volatile uint32_t mu_stat;
	volatile uint32_t mu_baud;
	volatile uint32_t UNUSED2[5];
	volatile uint32_t spi0_cntl0;
	volatile uint32_t spi0_cntl1;
	volatile uint32_t spi0_stat;
	volatile uint32_t UNUSED3[1];
	volatile uint32_t spi0_io;
	volatile uint32_t spi0_peek;
	volatile uint32_t UNUSED4[10];
	volatile uint32_t spi1_cntl0;
	volatile uint32_t spi1_cntl1;
	volatile uint32_t spi1_stat;
	volatile uint32_t UNUSED5[1];
	volatile uint32_t spi1_io;
	volatile uint32_t spi1_peek;
} aux_reg_t;

#define GPIO_REG (0x200000)
typedef struct {
	volatile uint32_t gpfsel0;
	volatile uint32_t gpfsel1;
	volatile uint32_t gpfsel2;
	volatile uint32_t gpfsel3;
	volatile uint32_t gpfsel4;
	volatile uint32_t gpfsel5;
	volatile uint32_t reserved1;
	volatile uint32_t gpset0;
	volatile uint32_t gpset1;
	volatile uint32_t reserved2;
	volatile uint32_t gpclr0;
	volatile uint32_t gpclr1;
	volatile uint32_t reserved3;
	volatile uint32_t gplev0;
	volatile uint32_t gplev1;
	volatile uint32_t reserved4;
	volatile uint32_t gpeds0;
	volatile uint32_t gpeds1;
	volatile uint32_t reserved5;
	volatile uint32_t gpren0;
	volatile uint32_t gpren1;
	volatile uint32_t reserved6;
	volatile uint32_t gpfen0;
	volatile uint32_t gpfen1;
	volatile uint32_t reserved7;
	volatile uint32_t gphen0;
	volatile uint32_t gphen1;
	volatile uint32_t reserved8;
	volatile uint32_t gplen0;
	volatile uint32_t gplen1;
	volatile uint32_t reserved9;
	volatile uint32_t gparen0;
	volatile uint32_t gparen1;
	volatile uint32_t reserved10;
	volatile uint32_t gpafen0;
	volatile uint32_t gpafen1;
	volatile uint32_t reserved11;
	volatile uint32_t gppud;
	volatile uint32_t gppudclk0;
	volatile uint32_t gppudclk1;
} gpio_reg_t;

#define UART_REG (0x201000)
typedef struct {
	volatile uint32_t dr;
	volatile uint32_t rsrecr;
	volatile uint32_t UNUSED1[5];
	volatile uint32_t fr;
	volatile uint32_t reserved1;
	volatile uint32_t ilpr;
	volatile uint32_t ibrd;
	volatile uint32_t fbrd;
	volatile uint32_t lcrh;
	volatile uint32_t cr;
	volatile uint32_t ifls;
	volatile uint32_t imsc;
	volatile uint32_t ris;
	volatile uint32_t mis;
	volatile uint32_t icr;
	volatile uint32_t dmacr;
	volatile uint32_t UNUSED2[13];
	volatile uint32_t itcr;
	volatile uint32_t itip;
	volatile uint32_t itop;
	volatile uint32_t tdr;
} uart_reg_t;

#define SYSTIMER_REG (0x3000)
typedef struct {
	volatile uint32_t cs;
	volatile uint32_t clo;
	volatile uint32_t chi;
	volatile uint32_t c0;
	volatile uint32_t c1;
	volatile uint32_t c2;
	volatile uint32_t c3;
} systimer_reg_t;


#define EMMC_REG (0x300000)
typedef struct {
	volatile uint32_t arg2;
	volatile uint32_t blksizecnt;
	volatile uint32_t arg1;
	volatile uint32_t cmdtm;
	volatile uint32_t resp0;
	volatile uint32_t resp1;
	volatile uint32_t resp2;
	volatile uint32_t resp3;
	volatile uint32_t data;
	volatile uint32_t status;
	volatile uint32_t control0;
	volatile uint32_t control1;
	volatile uint32_t interrupt;
	volatile uint32_t irpt_mask;
	volatile uint32_t irpt_en;
	volatile uint32_t control2;
	volatile uint32_t capabilities_0;
	volatile uint32_t capabilities_1;
	volatile uint32_t UNUSED1[2];
	volatile uint32_t force_irpt;
	volatile uint32_t UNUSED2[7];
	volatile uint32_t boot_timeout;
	volatile uint32_t dbg_sel;
	volatile uint32_t UNUSED3[2];
	volatile uint32_t exrdfifo_cfg;
	volatile uint32_t exrdfifo_en;
	volatile uint32_t tune_step;
	volatile uint32_t tune_steps_std;
	volatile uint32_t tune_steps_ddr;
	volatile uint32_t UNUSED4[23];
	volatile uint32_t spi_int_spt;
	volatile uint32_t UNUSED5[2];
	volatile uint32_t slotisr_ver;
} emmc_reg_t;

#endif /*__ASSEMBLY__*/

/*
 * QEMU(v4.1)只能模拟System Timer，不能模拟Timer(ARM side)
 * 但是，因为ARM Timer不稳定，建议用System Timer作为时钟
 */
#if RPI_QEMU == 1
#define IRQ_TIMER     9
#else
#define IRQ_TIMER     0	
#endif

#define NR_IRQ        (8+64)

#endif /* _CPU_H */
