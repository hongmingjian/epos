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
#ifndef _CPU_H
#define _CPU_H

#include "arch.h"

#define MMIO_BASE      0xC4000000

#define ARMTIMER_REG_BASE (MMIO_BASE+0xB400)
typedef struct {
	volatile unsigned int Load;
	volatile unsigned int Value;
	volatile unsigned int Control;
#define ARMTIMER_CTRL_23BIT            (1 << 1)
#define ARMTIMER_CTRL_PRESCALE_1       (0 << 2)
#define ARMTIMER_CTRL_PRESCALE_16      (1 << 2)
#define ARMTIMER_CTRL_PRESCALE_256     (2 << 2)
#define ARMTIMER_CTRL_INTR_ENABLE      (1 << 5)
#define ARMTIMER_CTRL_ENABLE           (1 << 7)

	volatile unsigned int IRQClear;
	volatile unsigned int RAWIRQ;
	volatile unsigned int MaskedIRQ;
	volatile unsigned int Reload;
	volatile unsigned int PreDivider;
	volatile unsigned int FreeRunningCounter;
} armtimer_reg_t;

#define INTR_REG_BASE (MMIO_BASE+0xB200)
typedef struct {
	volatile unsigned int IRQ_basic_pending;
	volatile unsigned int IRQ_pending_1;
	volatile unsigned int IRQ_pending_2;
	volatile unsigned int FIQ_control;
	volatile unsigned int Enable_IRQs_1;
	volatile unsigned int Enable_IRQs_2;
	volatile unsigned int Enable_basic_IRQs;
	volatile unsigned int Disable_IRQs_1;
	volatile unsigned int Disable_IRQs_2;
	volatile unsigned int Disable_basic_IRQs;
} intr_reg_t;

#define AUX_REG_BASE (MMIO_BASE+0x215000)
typedef struct {
	volatile unsigned int IRQ;
	volatile unsigned int enables;
	volatile unsigned int UNUSED1[14];
	volatile unsigned int mu_io;
	volatile unsigned int mu_ier;
	volatile unsigned int mu_iir;
	volatile unsigned int mu_lcr;
	volatile unsigned int mu_mcr;
	volatile unsigned int mu_lsr;
	volatile unsigned int mu_msr;
	volatile unsigned int mu_scratch;
	volatile unsigned int mu_cntl;
	volatile unsigned int mu_stat;
	volatile unsigned int mu_baud;
	volatile unsigned int UNUSED2[5];
	volatile unsigned int spi0_cntl0;
	volatile unsigned int spi0_cntl1;
	volatile unsigned int spi0_stat;
	volatile unsigned int UNUSED3[1];
	volatile unsigned int spi0_io;
	volatile unsigned int spi0_peek;
	volatile unsigned int UNUSED4[10];
	volatile unsigned int spi1_cntl0;
	volatile unsigned int spi1_cntl1;
	volatile unsigned int spi1_stat;
	volatile unsigned int UNUSED5[1];
	volatile unsigned int spi1_io;
	volatile unsigned int spi1_peek;
} aux_reg_t;

#define GPIO_REG_BASE (MMIO_BASE+0x200000)
typedef struct {
	volatile unsigned int gpfsel0;
	volatile unsigned int gpfsel1;
	volatile unsigned int gpfsel2;
	volatile unsigned int gpfsel3;
	volatile unsigned int gpfsel4;
	volatile unsigned int gpfsel5;
	volatile unsigned int reserved1;
	volatile unsigned int gpset0;
	volatile unsigned int gpset1;
	volatile unsigned int reserved2;
	volatile unsigned int gpclr0;
	volatile unsigned int gpclr1;
	volatile unsigned int reserved3;
	volatile unsigned int gplev0;
	volatile unsigned int gplev1;
	volatile unsigned int reserved4;
	volatile unsigned int gpeds0;
	volatile unsigned int gpeds1;
	volatile unsigned int reserved5;
	volatile unsigned int gpren0;
	volatile unsigned int gpren1;
	volatile unsigned int reserved6;
	volatile unsigned int gpfen0;
	volatile unsigned int gpfen1;
	volatile unsigned int reserved7;
	volatile unsigned int gphen0;
	volatile unsigned int gphen1;
	volatile unsigned int reserved8;
	volatile unsigned int gplen0;
	volatile unsigned int gplen1;
	volatile unsigned int reserved9;
	volatile unsigned int gparen0;
	volatile unsigned int gparen1;
	volatile unsigned int reserved10;
	volatile unsigned int gpafen0;
	volatile unsigned int gpafen1;
	volatile unsigned int reserved11;
	volatile unsigned int gppud;
	volatile unsigned int gppudclk0;
	volatile unsigned int gppudclk1;
} gpio_reg_t;

#define IRQ_TIMER     0
#define NR_IRQ        (8+64)

#endif /* _CPU_H */
