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
    .extern _edata, _end
    .extern _abort_handler, _syscall, _undefined_handler, _irq_handler
	.extern _cstart, _schedule, _g_resched, _g_task_running
    .extern _cpuid

#include "machdep.h"

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
	add		r13, r13, #IRQ_SIZE;		\
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

#define PROLOGUE \
	ldr r0, =_g_task_running; \
    ldr r0, [r0]; \
	cmp r0, #0;  \
	beq 1f; \
    ; \
	ldr r1, [sp, #CF_SPSR]; \
	and	r1, r1, #PSR_MODE_MASK; \
	cmp r1, #PSR_MODE_USR; \
	bne 1f; \
	; \
	ldr r0, [r0]; \
	sub r0, r0, #FRAME_SIZE; \
	mov r1, sp; \
	mov sp, r0; \
    ; \
	ldmia r1!, {r2-r11}; \
	stmia r0!, {r2-r11}; \
	ldmia r1!, {r2-r10}; \
	stmia r0!, {r2-r10}; \
    ; \
1:

#define EPILOGUE \
	ldr r0, =_g_task_running; \
    ldr r0, [r0]; \
	cmp r0, #0;  \
	beq 1f; \
    ; \
	ldr r1, [sp, #CF_SPSR]; \
	and	r1, r1, #PSR_MODE_MASK; \
	cmp r1, #PSR_MODE_USR; \
	bne 1f; \
	; \
    add r1, r0, #4096; \
    str r1, [r0]; \
	ldr r0, =(KERNBASE+0x1000-36); \
	sub r0, r0, #FRAME_SIZE; \
	mov r1, sp; \
	mov sp, r0; \
    ; \
	ldmia r1!, {r2-r11}; \
	stmia r0!, {r2-r11}; \
	ldmia r1!, {r2-r10}; \
	stmia r0!, {r2-r10}; \
    ; \
1:

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

vector_reset:
	.word R(reset)
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
	;@read cpuid
    mrc p15, #0, r12, c0, c0, #0

    mrs r10, cpsr
 	and r11, r10, #PSR_MODE_MASK
 	cmp r11, #PSR_MODE_HYP
	bne rolling
	bic r10, r10, #PSR_MODE_MASK
	orr r10, r10, #PSR_MODE_SVC
	orr r10, r10, #(PSR_A|PSR_I|PSR_F)
    msr spsr_cxsf, r10
    add lr, pc, #4
	.long 0xE12EF30E	             ;@msr ELR_hyp,lr
	.long 0xE160006E	             ;@eret

rolling:
	;@copy exception vector table and related vector_* to 0
	mov r10, #LOADADDR
	mov	r11, #0x0000
	ldmia r10!, {r4-r9}
	stmia r11!, {r4-r9}
	ldmia r10!, {r4-r9}
	stmia r11!, {r4-r9}
	ldmia r10!, {r4-r7}
	stmia r11!, {r4-r7}

	;@zeroify bss
	ldr r10, =R(_edata)
	ldr r11, =R(_end)
	sub r10, r10, #KERNBASE
	sub r11, r11, #KERNBASE
	ldr r9, =0x0
zeroify:
	cmp		r11, r10
	ble		zeroify.1
	stmia	r10!, {r9}
	b		zeroify
zeroify.1:

	;@must be after zeroifying bss
    ldr r11, =R(_cpuid)
    str r12, [r11]

	;@setup stack for modes to be used
	mrs	r11, cpsr
	bic	r11, r11, #PSR_MODE_MASK

	orr	r12, r11, #PSR_MODE_UND|PSR_I|PSR_F
	msr	cpsr_cxsf, r12
	ldr	sp, =(KERNBASE+0x1000)

	orr	r12, r11, #PSR_MODE_ABT|PSR_I|PSR_F
	msr	cpsr_cxsf, r12
	ldr	sp, =(KERNBASE+0x1000-12)

	orr	r12, r11, #PSR_MODE_IRQ|PSR_I|PSR_F
	msr	cpsr_cxsf, r12
	ldr	sp, =(KERNBASE+0x1000-24)

	orr	r12, r11, #PSR_MODE_SVC|PSR_I|PSR_F
	msr	cpsr_cxsf, r12
	ldr sp, =(0x1000-36)

	;@enable
    mrc p15, #0, r12, c1, c0, #0
    orr r12, r12, #(1<<11)           ;@branch prediction
    orr r12, r12, #(1<<12)           ;@l1 icache
    mcr p15, #0, r12, c1, c0, #0

    /* Now enter the C main function... */
    b _cstart

prefetch_abort:
	PUSHFRAMEINSVC
    PROLOGUE

	mov	r0, sp
    mrc p15, #0, r1, c6, c0, #2      ;@get IFAR
    mrc p15, #0, r2, c5, c0, #1      ;@get IFSR

	bl _sti
	bl _abort_handler
	bl _cli

    b _ret_from_syscall

data_abort:
    ;@https://stackoverflow.com/questions/6292620/why-are-the-return-addresses-of-prefetch-abort-and-data-abort-different-in-arm-e
	sub lr, lr, #4
	PUSHFRAMEINSVC
    PROLOGUE

	mov	r0, sp
    mrc p15, #0, r1, c6, c0, #0      ;@get DFAR
    mrc p15, #0, r2, c5, c0, #0      ;@get DFSR

	bl _sti
	bl _abort_handler
	bl _cli

    b _ret_from_syscall

irq:
	PUSHFRAMEINSVC
    PROLOGUE

	mov	r0, sp
	bl _irq_handler

	ldr r0, =_g_resched
    ldr r0, [r0]
    cmp r0, #0
    blne _schedule

    b _ret_from_syscall

undefined:
	PUSHFRAMEINSVC
    PROLOGUE

	mov r0, sp
	bl _undefined_handler

    b _ret_from_syscall

swi:
	sub	sp, sp, #FRAME_SIZE
	str	lr, [sp, #CF_PC]
	mrs	lr, spsr
	str	lr, [sp, #CF_SPSR]
	add	r14, sp, #4
	stmia r14!, {r0-r12}
	stmia r14, {r13, r14}^
	str	sp, [sp, #CF_SVC_SP]

    PROLOGUE

	mov	r0, sp
	bl _sti
	bl _syscall
	bl _cli

	.globl _ret_from_syscall
_ret_from_syscall:
    EPILOGUE
	PULLFRAMEANDEXIT

	.globl	_sti, _cli
_sti:							     ;@enable IRQs
	stmfd	sp!, {r0, lr}
	mrs		r0, cpsr
	bic		r0, r0, #PSR_I
	msr		cpsr_cxsf, r0
	ldmfd	sp!, {r0, pc}

_cli:							     ;@disable IRQs
	stmfd	sp!, {r0, lr}
	mrs		r0, cpsr
	orr		r0, r0, #PSR_I
	msr		cpsr_cxsf, r0
	ldmfd	sp!, {r0, pc}

	.globl _atomic_or, _atomic_and, _atomic_xor
_atomic_or:
	stmfd	sp!, {r2, r3, lr}
	mrs	r2, cpsr
	orr	r3, r2, #PSR_I|PSR_F
	msr	cpsr_c, r3

	ldr	r3, [r0]
	orr	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_c, r2
	ldmfd	sp!, {r2, r3, pc}

_atomic_and:
	stmfd	sp!, {r2, r3, lr}
	mrs	r2, cpsr
	orr	r3, r2, #PSR_I|PSR_F
	msr	cpsr_c, r3

	ldr	r3, [r0]
	and	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_c, r2
	ldmfd	sp!, {r2, r3, pc}

_atomic_xor:
	stmfd	sp!, {r2, r3, lr}
	mrs	r2, cpsr
	orr	r3, r2, #PSR_I|PSR_F
	msr	cpsr_c, r3

	ldr	r3, [r0]
	eor	r3, r3, r1
	str	r3, [r0]

	msr	cpsr_c, r2
	ldmfd	sp!, {r2, r3, pc}

	.globl _invlpg
_invlpg:
	mcr p15, #0, r0, c8, c7, #1
	mcr p15, #0, r0, c8, c6, #1
	mov pc, lr

	.globl	___aeabi_idivmod
___aeabi_idivmod:
	stmfd	sp!, {r0, r1, ip, lr}
	bl	___aeabi_idiv
	ldmfd	sp!, {r1, r2, ip, lr}
	mul	r3, r0, r2
	sub	r1, r1, r3
	mov	pc, lr

    .globl _enable_l1_dcache
_enable_l1_dcache:
    push {r0}
    mrc p15, #0, r0, c1, c0, #0
    orr r0, r0, #(1<<2)              ;@data and unified caches
    mcr p15, #0, r0, c1, c0, #0
    pop {r0}
    mov pc, lr

    .globl _cpu_idle
_cpu_idle:
	mcr p15, #0, r0, c7, c0, #4
    mov pc, lr
	