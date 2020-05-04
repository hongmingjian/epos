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
#define CPUID_BCM2836  0x410FC075
#define CPUID_BCM2837  0x410FD034
#define CPUID_BCM2711  0x410FD083
#if RPI_QEMU == 1
#define CPUID_QEMU     0x410FC075
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

#define MAILBOX_REG (0xB880)
typedef struct {
    volatile uint32_t read0;
    volatile uint32_t UNUSED1[3];
    volatile uint32_t peek0;
    volatile uint32_t sender0;
    volatile uint32_t status0;
    volatile uint32_t config0;
    volatile uint32_t write1;
    volatile uint32_t UNUSED2[3];
    volatile uint32_t peek1;
    volatile uint32_t sender1;
    volatile uint32_t status1;
    volatile uint32_t config1;
} mailbox_reg_t;

/*--------------------------------------------------------------------------}
{                       ENUMERATED MAILBOX CLOCK ID                         }
{         https://github.com/raspberrypi/firmware/wiki/Mailboxes            }
{--------------------------------------------------------------------------*/
typedef enum {
    CLOCK_EMMC      = 0x1,//EMMC
    CLOCK_UART      = 0x2,//UART
    CLOCK_ARM       = 0x3,//ARM
    CLOCK_CORE      = 0x4,//CORE
    CLOCK_V3D       = 0x5,//V3D
    CLOCK_H264      = 0x6,//H264
    CLOCK_ISP       = 0x7,//ISP
    CLOCK_SDRAM     = 0x8,//SDRAM
    CLOCK_PIXEL     = 0x9,//PIXEL
    CLOCK_PWM       = 0xA,//PWM
} MAILBOX_CLOCK;

/*--------------------------------------------------------------------------}
{                     ENUMERATED MAILBOX CHANNELS                           }
{         https://github.com/raspberrypi/firmware/wiki/Mailboxes            }
{--------------------------------------------------------------------------*/
typedef enum {
    CHANNEL_POWER   = 0x0,//Power Management
    CHANNEL_FB      = 0x1,//Frame Buffer
    CHANNEL_VUART   = 0x2,//Virtual UART
    CHANNEL_VCHIQ   = 0x3,//VCHIQ
    CHANNEL_LEDS    = 0x4,//LEDs
    CHANNEL_BUTTONS = 0x5,//Buttons
    CHANNEL_TOUCH   = 0x6,//Touchscreen
    CHANNEL_COUNT   = 0x7,//Counter
    CHANNEL_TAGS    = 0x8,//Tags (ARM to VC)
    CHANNEL_GPU     = 0x9,//GPU (VC to ARM)
} MAILBOX_CHANNEL;

/*--------------------------------------------------------------------------}
{                    ENUMERATED MAILBOX POWER ID                            }
{  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface  }
{--------------------------------------------------------------------------*/
typedef enum {
    POWER_SDCARD = 0x0,//SD CARD
    POWER_UART0  = 0x1,//UART0
    POWER_UART1  = 0x2,//UART1
    POWER_USBHCD = 0x3,//USB HCD
    POWER_I2C0   = 0x4,//I2C channel 0
    POWER_I2C1   = 0x5,//I2C channel 1
    POWER_I2C2   = 0x6,//I2C channel 2
    POWER_SPI    = 0x7,//SPI
    POWER_CCP2TX = 0x8,//CCP2TX
} MAILBOX_POWER;

/*--------------------------------------------------------------------------}
{               ENUMERATED MAILBOX TAG CHANNEL COMMANDS                     }
{  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface  }
{--------------------------------------------------------------------------*/
typedef enum {
    /* Videocore info commands */
    TAG_GET_FIRMWARE_VERSION     = 0x00000001,//Get firmware revision

    /* Hardware info commands */
    TAG_GET_BOARD_MODEL          = 0x00010001,//Get board model
    TAG_GET_BOARD_REVISION       = 0x00010002,//Get board revision
    TAG_GET_BOARD_MAC_ADDRESS    = 0x00010003,//Get board MAC address
    TAG_GET_BOARD_SERIAL         = 0x00010004,//Get board serial
    TAG_GET_ARM_MEMORY           = 0x00010005,//Get ARM memory
    TAG_GET_VC_MEMORY            = 0x00010006,//Get VC memory
    TAG_GET_CLOCKS               = 0x00010007,//Get clocks

    /* Power commands */
    TAG_GET_POWER_STATE          = 0x00020001,//Get power state
    TAG_GET_TIMING               = 0x00020002,//Get timing
    
    TAG_SET_POWER_STATE          = 0x00028001,//Set power state

    /* GPIO commands */
    TAG_GET_GET_GPIO_STATE       = 0x00030041,//Get GPIO state
    TAG_SET_GPIO_STATE           = 0x00038041,//Set GPIO state

    /* Clock commands */
    TAG_GET_CLOCK_STATE          = 0x00030001,//Get clock state
    TAG_GET_CLOCK_RATE           = 0x00030002,//Get clock rate
    TAG_GET_MAX_CLOCK_RATE       = 0x00030004,//Get max clock rate
    TAG_GET_MIN_CLOCK_RATE       = 0x00030007,//Get min clock rate
    TAG_GET_TURBO                = 0x00030009,//Get turbo

    TAG_SET_CLOCK_STATE          = 0x00038001,//Set clock state
    TAG_SET_CLOCK_RATE           = 0x00038002,//Set clock rate
    TAG_SET_TURBO                = 0x00038009,//Set turbo

    /* Voltage commands */
    TAG_GET_VOLTAGE              = 0x00030003,//Get voltage
    TAG_GET_MAX_VOLTAGE          = 0x00030005,//Get max voltage
    TAG_GET_MIN_VOLTAGE          = 0x00030008,//Get min voltage

    TAG_SET_VOLTAGE              = 0x00038003,//Set voltage

    /* Temperature commands */
    TAG_GET_TEMPERATURE          = 0x00030006,//Get temperature
    TAG_GET_MAX_TEMPERATURE      = 0x0003000A,//Get max temperature

    /* Memory commands */
    TAG_ALLOCATE_MEMORY          = 0x0003000C,//Allocate Memory
    TAG_LOCK_MEMORY              = 0x0003000D,//Lock memory
    TAG_UNLOCK_MEMORY            = 0x0003000E,//Unlock memory
    TAG_RELEASE_MEMORY           = 0x0003000F,//Release Memory

    /* Execute code commands */
    TAG_EXECUTE_CODE             = 0x00030010,//Execute code

    /* QPU control commands */
    TAG_EXECUTE_QPU              = 0x00030011,//Execute code on QPU
    TAG_ENABLE_QPU               = 0x00030012,//QPU enable

    /* Displaymax commands */
    TAG_GET_DISPMANX_HANDLE      = 0x00030014,//Get displaymax handle
    TAG_GET_EDID_BLOCK           = 0x00030020,//Get HDMI EDID block

    /* SD Card commands */
    GET_SDHOST_CLOCK             = 0x00030042,//Get SD Card EMCC clock
    SET_SDHOST_CLOCK             = 0x00038042,//Set SD Card EMCC clock

    /* Framebuffer commands */
    TAG_ALLOCATE_FRAMEBUFFER     = 0x00040001,//Allocate Framebuffer address
    TAG_BLANK_SCREEN             = 0x00040002,//Blank screen
    TAG_GET_PHYSICAL_WIDTH_HEIGHT= 0x00040003,//Get physical screen width/height
    TAG_GET_VIRTUAL_WIDTH_HEIGHT = 0x00040004,//Get virtual screen width/height
    TAG_GET_COLOUR_DEPTH         = 0x00040005,//Get screen colour depth
    TAG_GET_PIXEL_ORDER          = 0x00040006,//Get screen pixel order
    TAG_GET_ALPHA_MODE           = 0x00040007,//Get screen alpha mode
    TAG_GET_PITCH                = 0x00040008,//Get screen line to line pitch
    TAG_GET_VIRTUAL_OFFSET       = 0x00040009,//Get screen virtual offset
    TAG_GET_OVERSCAN             = 0x0004000A,//Get screen overscan value
    TAG_GET_PALETTE              = 0x0004000B,//Get screen palette

    TAG_RELEASE_FRAMEBUFFER      = 0x00048001,//Release Framebuffer address
    TAG_SET_PHYSICAL_WIDTH_HEIGHT= 0x00048003,//Set physical screen width/heigh
    TAG_SET_VIRTUAL_WIDTH_HEIGHT = 0x00048004,//Set virtual screen width/height
    TAG_SET_COLOUR_DEPTH         = 0x00048005,//Set screen colour depth
    TAG_SET_PIXEL_ORDER          = 0x00048006,//Set screen pixel order
    TAG_SET_ALPHA_MODE           = 0x00048007,//Set screen alpha mode
    TAG_SET_VIRTUAL_OFFSET       = 0x00048009,//Set screen virtual offset
    TAG_SET_OVERSCAN             = 0x0004800A,//Set screen overscan value
    TAG_SET_PALETTE              = 0x0004800B,//Set screen palette
    TAG_SET_VSYNC                = 0x0004800E,//Set screen VSync
    TAG_SET_BACKLIGHT            = 0x0004800F,//Set screen backlight

    /* VCHIQ commands */
    TAG_VCHIQ_INIT               = 0x00048010,//Enable VCHIQ

    /* Config commands */
    TAG_GET_COMMAND_LINE         = 0x00050001,//Get command line

    /* Shared resource management commands */
    TAG_GET_DMA_CHANNELS         = 0x00060001,//Get DMA channels

    /* Cursor commands */
    TAG_SET_CURSOR_INFO          = 0x00008010,//Set cursor info
    TAG_SET_CURSOR_STATE         = 0x00008011,//Set cursor state
} MAILBOX_TAG;

#define MAILBOX_REQUEST     0x00000000
#define MAILBOX_RESPONSE    0x80000000
#define MAILBOX_FULL        0x80000000
#define MAILBOX_EMPTY       0x40000000

#define RNG_REG (0x104000)
typedef struct {
    volatile uint32_t ctrl;
    volatile uint32_t status;
    volatile uint32_t data;
    volatile uint32_t UNUSED1[1];
    volatile uint32_t intmask;
} rng_reg_t;

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
