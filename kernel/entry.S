#
# vim: filetype=asm:fenc=utf-8:ts=4:et:sw=4:sts=4
#
# Copyright (C) 2005, 2013 Hong MingJian<hongmingjian@gmail.com>
# All rights reserved.
#
# This file is part of the EPOS.
#
# Redistribution and use in source and binary forms are freely
# permitted provided that the above copyright notice and this
# paragraph and the following disclaimer are duplicated in all
# such forms.
#
# This software is provided "AS IS" and without any express or
# implied warranties, including, without limitation, the implied
# warranties of merchantability and fitness for a particular
# purpose.
#
# $Id: entry.S,v 1.1.1.1 2007/06/22 03:20:53 hmj Exp $
#
    .extern _edata
    .extern _end
	.extern _cstart
    .extern _abort_handler, _swi_handler, _undefined_handler, _irq_handler

#define KERNBASE 0xC0000000

#define PSR_MODE_USR  0x10
#define PSR_MODE_FIQ  0x11
#define PSR_MODE_IRQ  0x12
#define PSR_MODE_SVC  0x13
#define PSR_MODE_ABT  0x17
#define PSR_MODE_UND  0x1b
#define PSR_MODE_SYS  0x1f
#define PSR_MODE_MASK 0x1f

#define PSR_N		  0x80000000
#define PSR_Z		  0x40000000
#define PSR_C		  0x20000000
#define PSR_V		  0x10000000
#define PSR_I		  0x80
#define PSR_F		  0x40
#define PSR_T		  0x20

#define CF_SPSR		0
#define CF_R0		4
#define CF_R1		8
#define CF_R2		12
#define CF_R3		16
#define CF_R4		20
#define CF_R5		24
#define CF_R6		28
#define CF_R7		32
#define CF_R8		36
#define CF_R9		40
#define CF_R10		44
#define CF_R11		48
#define CF_R12		52
#define CF_USR_SP	56
#define CF_USR_LR	60
#define CF_SVC_SP	64
#define CF_SVC_LR	68
#define CF_PC		72

#define FRAME_SIZE	76

#define	IRQ_R0		0
#define	IRQ_SPSR	4
#define	IRQ_LR		8
#define	IRQ_SIZE	12

#define PUSHFRAMEINSVC					\
	sub		lr, lr, #4;					\
	sub		r13, r13, #IRQ_SIZE;		\
	str		lr, [r13, #IRQ_LR];			\
	mrs		lr, spsr;					\
	str		lr, [r13, #IRQ_SPSR];		\
	str		r0, [r13, #IRQ_R0];			\
	mov		r0, r13;					\
	add		r13, r13, #12;				\
	mrs		r14, cpsr;					\
	bic		r14, r14, #PSR_MODE_MASK;	\
	orr		r14, r14, #PSR_MODE_SVC;	\
	msr		cpsr_cxsf, r14;				\
	sub		sp, sp, #FRAME_SIZE;		\
	str		lr, [sp, #CF_SVC_LR];		\
	ldr		lr, [r0, #IRQ_LR];			\
	str		lr, [sp, #CF_PC];			\
	ldr		r14, [r0, #IRQ_SPSR];		\
	str		r14, [sp, #CF_SPSR];		\
	ldr		r0, [r0, #IRQ_R0];			\
	add		r14, sp, #4;				\
	stmia	r14!, {r0-r12};				\
	stmia	r14, {r13, r14}^;			\
	str		sp, [sp, #CF_SVC_SP]

#define PULLFRAMEANDEXIT				\
	ldmia	sp!, {r0};					\
	msr		spsr, r0;					\
	ldmia	sp!, {r0-r12};				\
	ldmia	sp, {r13, r14}^;			\
	add		sp, sp, #(3*4);				\
	ldmia	sp!, {lr, pc}^




    .text
    .globl _entry
_entry:
    ldr pc, vector_reset
    ldr pc, vector_undefined
    ldr pc, vector_swi
    ldr pc, vector_prefetch_abort
    ldr pc, vector_data_abort
    b   .
    ldr pc, vector_irq
    b   .

    .align 4

vector_reset:
	.word reset
vector_undefined:
	.word undefined
vector_swi:
	.word swi
vector_prefetch_abort:
	.word prefetch_abort
vector_data_abort:
	.word data_abort
vector_reserved:
	.word 0
vector_irq:
	.word irq
vector_fiq:
	.word 0

reset:
   	mov r0, #0x8000
  	mov	r1, #0x0000
  	ldmia r0!, {r2, r3, r4, r5, r6, r7, r8, r9}
  	stmia r1!, {r2, r3, r4, r5, r6, r7, r8, r9}
  	ldmia r0!, {r2, r3, r4, r5, r6, r7, r8, r9}
  	stmia r1!, {r2, r3, r4, r5, r6, r7, r8, r9}

	@ zeroify bss
	ldr r0, =_edata
	ldr r1, =_end
	sub r0, r0, #KERNBASE
	sub r1, r1, #KERNBASE
	ldr r2, =0x0
zeroify:
	cmp		r1, r0
	ble		zeroify.1
	stmia	r0!, {r2}
	b		zeroify
zeroify.1:

	mrs	r0, cpsr
	bic	r0, r0, #PSR_MODE_MASK

	orr	r1, r0, #PSR_MODE_UND|PSR_I|PSR_F
	msr	cpsr_cxsf, r1
	ldr	sp, =0x4000

	orr	r1, r0, #PSR_MODE_FIQ|PSR_I|PSR_F
	msr	cpsr_cxsf, r1
	ldr	sp, =(0x4000-0x100)

	orr	r1, r0, #PSR_MODE_ABT|PSR_I|PSR_F
	msr	cpsr_cxsf, r1
	ldr	sp, =(0x4000-0x200)

	orr	r1, r0, #PSR_MODE_IRQ|PSR_I|PSR_F
	msr	cpsr_cxsf, r1
	ldr	sp, =(0x4000-0x300)

	orr	r1, r0, #PSR_MODE_SVC|PSR_I|PSR_F
	msr	cpsr_cxsf, r1
	ldr r0, =(0x4000-0x400)
	sub sp, r0, #4

    /* Now enter the C main function... */
    bl _cstart

halt:
	wfe
	b halt

prefetch_abort:
	PUSHFRAMEINSVC
	mov		r0, sp

    mrc  p15, 0, r1, c6, c0, 2        @get IFAR
    mrc  p15, 0, r2, c5, c0, 1        @get IFSR

	bl _abort_handler

	PULLFRAMEANDEXIT

data_abort:
	PUSHFRAMEINSVC
	mov		r0, sp

    mrc p15, 0, r1, c6, c0, 0         @get DFAR
    mrc p15, 0, r2, c5, c0, 0         @get DFSR

	bl _abort_handler

	PULLFRAMEANDEXIT

irq:
	PUSHFRAMEINSVC
	mov		r0, sp

	bl		_irq_handler

	PULLFRAMEANDEXIT

undefined:
	PUSHFRAMEINSVC
	mov		r0, sp

	bl		_undefined_handler

	PULLFRAMEANDEXIT
swi:
	PUSHFRAMEINSVC
	mov		r0, sp

	bl		_swi_handler

	PULLFRAMEANDEXIT

	.globl	_sti, _cli
_sti:							@ enable IRQs
	stmfd	sp!, {r0, lr}
	mrs		r0, cpsr
	bic		r0, r0, #PSR_I
	msr		cpsr_cxsf, r0
	ldmfd	sp!, {r0, pc}

_cli:							@ disable IRQs
	stmfd	sp!, {r0, lr}
	mrs		r0, cpsr
	orr		r0, r0, #PSR_I
	msr		cpsr_cxsf, r0
	ldmfd	sp!, {r0, pc}
	
	.globl _atomic_or, _atomic_and, _atomic_xor
_atomic_or:
	stmfd	sp!, {r2, r3, lr}
	mrs	r2, cpsr
	orr	r3, r2, #(PSR_I)
	msr	cpsr_c, r3

	ldr	r3, [r0]
	orr	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_c, r2
	ldmfd	sp!, {r2, r3, pc}
	
_atomic_and:
	stmfd	sp!, {r2, r3, lr}
	mrs	r2, cpsr
	orr	r3, r2, #(PSR_I)
	msr	cpsr_c, r3

	ldr	r3, [r0]
	and	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_c, r2
	ldmfd	sp!, {r2, r3, pc}
	
_atomic_xor:
	stmfd	sp!, {r2, r3, lr}
	mrs	r2, cpsr
	orr	r3, r2, #(PSR_I)
	msr	cpsr_c, r3

	ldr	r3, [r0]
	eor	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_c, r2
	ldmfd	sp!, {r2, r3, pc}
	
	.globl _invlpg
_invlpg:
	stmfd	sp!, {r3, lr}
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		@drain WB
	mcr	p15, 0, r0, c8, c6, 1		@invalidate D TLB entry
	mcr p15, 0, r0, c8, c5, 1		@invalidate I TLB entry
	ldmfd	sp!, {r3, pc}
