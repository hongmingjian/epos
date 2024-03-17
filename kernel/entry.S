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
    .extern _g_intr_vector
    .extern _g_resched
    .extern _schedule
    .extern _g_task_running
    .extern _syscall
    .extern _exception


#define KERNBASE 0xC0000000
#define R(foo) ((foo)-KERNBASE)

#define KDSEL 0x10
#define KCSEL 0x8

#include "multiboot.h"

/* The flags for the Multiboot header. */
# define MULTIBOOT_HEADER_FLAGS         0x00010003

    .text
    .code32
    .globl _entry
_entry:
    jmp     multiboot_entry

/* Align 32 bits boundary. */
    .align  4

multiboot_header:
    .long   MULTIBOOT_HEADER_MAGIC /* magic */
    .long   MULTIBOOT_HEADER_FLAGS /* flags */
    .long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) /* checksum */
    .long   R(multiboot_header) /* header_addr */
    .long   R(_entry) /* load_addr */
    .long   R(_edata) /* load_end_addr */
    .long   R(_end) /* bss_end_addr */
    .long   R(_entry) /* entry_addr */

multiboot_entry:
    /* Switch to the kernel stack */
    movl $R(_tmp_stack), %esp

    /* Reset EFLAGS. */
    pushl $0x2
    popfl

    /* Push the pointer to the Multiboot information structure. */
    pushl   %ebx
    /* Push the magic value. */
    pushl   %eax

    /* Now enter the C main function... */
    call _cstart

    die:
    hlt
    jmp    die

    .globl _exception_divide_error
_exception_divide_error:
    pushl $0
    pushl $0
    jmp exception_common

    .globl _exception_debug
_exception_debug:
    pushl $0
    pushl $1
    jmp exception_common

    .globl _exception_nmi
_exception_nmi:
    pushl $0
    pushl $2
    jmp exception_common

    .globl _exception_breakpoint
_exception_breakpoint:
    pushl $0
    pushl $3
    jmp exception_common

    .globl _exception_overflow
_exception_overflow:
    pushl $0
    pushl $4
    jmp exception_common

    .globl _exception_bounds_check
_exception_bounds_check:
    pushl $0
    pushl $5
    jmp exception_common

    .globl _exception_inval_opcode
_exception_inval_opcode:
    pushl $0
    pushl $6
    jmp exception_common

    .globl _exception_copr_not_avail
_exception_copr_not_avail:
    pushl $0
    pushl $7
    jmp exception_common

    .globl _exception_double_fault
_exception_double_fault:
    pushl $8
    jmp exception_common

    .globl _exception_copr_seg_overrun
_exception_copr_seg_overrun:
    pushl $0
    pushl $9
    jmp exception_common

    .globl _exception_inval_tss
_exception_inval_tss:
    pushl $10
    jmp exception_common

    .globl _exception_segment_not_present
_exception_segment_not_present:
    pushl $11
    jmp exception_common

    .globl _exception_stack_fault
_exception_stack_fault:
    pushl $12
    jmp exception_common

    .globl _exception_general_protection
_exception_general_protection:
    pushl $13
    jmp exception_common

    .globl _exception_page_fault
_exception_page_fault:
    pushl $14
    jmp exception_common

    .globl _exception_intel_reserved
_exception_intel_reserved:
    pushl $0
    pushl $15
    jmp exception_common

    .globl _exception_copr_error
_exception_copr_error:
    pushl $0
    pushl $16
    jmp exception_common

    .globl _exception_alignment_check
_exception_alignment_check:
    pushl $17
    jmp exception_common

    .globl _exception_machine_check
_exception_machine_check:
    pushl $0
    pushl $18
    jmp exception_common

    .globl _exception_simd_fp
_exception_simd_fp:
    pushl $0
    pushl $19
    jmp exception_common

#define PROLOGUE \
    movl _g_task_running, %eax; \
    cmpl $0, %eax; \
    je 3f; \
    ; \
    movl 60(%esp), %ebx; \
    testl $0x20000, %ebx; \
    jz 1f; \
    movl $22, %ecx; \
    jmp 2f; \
    1:; \
    movl 56(%esp), %ebx; \
    testl $3, %ebx; \
    jz 3f; \
    movl $18, %ecx; \
    2:; \
    movl %ecx, %edx; \
    shll $2, %edx; \
    movl %esp, %esi; \
    movl (%eax), %esp; \
    subl %edx, %esp; \
    movl %esp, %edi; \
    cld; \
    rep movsl; \
    3:

#define EPILOGUE \
    movl _g_task_running, %eax; \
    cmpl $0, %eax; \
    je 3f; \
    ; \
    movl %esp, %ecx; \
    addl $(16*4), %ecx; \
    movl 60(%esp), %ebx; \
    testl $0x20000, %ebx; \
    jz 1f; \
    addl $(6*4), %ecx; \
    jmp 2f; \
    1:; \
    movl 56(%esp), %ebx; \
    testl $3, %ebx; \
    jz 2f; \
    addl $(2*4), %ecx; \
    2:; \
    movl %ecx, (%eax); \
    3:

exception_common:
    pushal
    pushl %fs
    pushl %ds
    pushl %es

    movl $KDSEL, %eax
    movw %ax, %fs
    movw %ax, %ds
    movw %ax, %es

    PROLOGUE

    pushl %esp
    call _exception
    addl $4, %esp

    movl 44(%esp), %ebx
    cmpl $0xd, %ebx
    jne 4f
    movl 60(%esp), %ebx
    testl $0x20000, %ebx
    jz 4f
    cmpl $0, %eax
    je 4f

    movl %esp, %esi
    movl $22, %ecx
    addl $(22*4), %esp
    movl 20(%esp), %edi
    cld
    rep movsl

    popl %edi
    popl %esi
    popl %ebx
    popl %ebp

    ret # return from sys_vm86

4:
    jmp _ret_from_syscall

    .globl _int0x82_syscall
_int0x82_syscall:
    subl $8, %esp # fake exception and errorcode

    pushal
    pushl %fs
    pushl %ds
    pushl %es

    movl $KDSEL, %eax
    movw %ax, %fs
    movw %ax, %ds
    movw %ax, %es

    PROLOGUE

    pushl %esp
    sti
    call _syscall
    cli
    addl $4, %esp

    .globl _ret_from_syscall
_ret_from_syscall:

    EPILOGUE

    popl %es
    popl %ds
    popl %fs
    popal
    addl $8, %esp # discard exception and errorcode
    iret

#define ENABLE_ICU1 \
    movb  $0x20, %al; \
    outb  %al, $0x20

#define ENABLE_ICU1_AND_2 \
    ENABLE_ICU1; \
    outb  %al, $0xa0

#define HWINT(irq, enable_icus) \
    subl $8, %esp; /*fake exception and errorcode*/\
    ; \
    pushal; \
    pushl %fs; \
    pushl %ds; \
    pushl %es; \
    ; \
    movl $KDSEL, %eax; \
    movw %ax, %fs; \
    movw %ax, %ds; \
    movw %ax, %es; \
    ; \
    PROLOGUE; \
    ; \
    pushl %esp; \
    pushl $irq; \
    movl (_g_intr_vector + 4 * irq), %eax; \
    call *%eax; \
    addl $8, %esp; \
    ; \
    enable_icus; \
    ; \
    cmpl $0, _g_resched; \
    je 4f; \
    call _schedule; \
    4:; \
    jmp _ret_from_syscall

    .globl _hwint00
_hwint00:
    HWINT(0, ENABLE_ICU1)

    .globl _hwint01
_hwint01:
    HWINT(1, ENABLE_ICU1)

    .globl _hwint02
_hwint02:
    HWINT(2, ENABLE_ICU1)

    .globl _hwint03
_hwint03:
    HWINT(3, ENABLE_ICU1)

    .globl _hwint04
_hwint04:
    HWINT(4, ENABLE_ICU1)

    .globl _hwint05
_hwint05:
    HWINT(5, ENABLE_ICU1)

    .globl _hwint06
_hwint06:
    HWINT(6, ENABLE_ICU1)

    .globl _hwint07
_hwint07:
    HWINT(7, ENABLE_ICU1)

    .globl _hwint08
_hwint08:
    HWINT(8, ENABLE_ICU1_AND_2)

    .globl _hwint09
_hwint09:
    HWINT(9, ENABLE_ICU1_AND_2)

    .globl _hwint10
_hwint10:
    HWINT(10, ENABLE_ICU1_AND_2)

    .globl _hwint11
_hwint11:
    HWINT(11, ENABLE_ICU1_AND_2)

    .globl _hwint12
_hwint12:
    HWINT(12, ENABLE_ICU1_AND_2)

    .globl _hwint13
_hwint13:
    HWINT(13, ENABLE_ICU1_AND_2)

    .globl _hwint14
_hwint14:
    HWINT(14, ENABLE_ICU1_AND_2)

    .globl _hwint15
_hwint15:
    HWINT(15, ENABLE_ICU1_AND_2)

#
# `lgdt' comes from FreeBSD
#
    .globl _lgdt
_lgdt:
    movl 4(%esp),%eax
    lgdt (%eax)

# flush the prefetch q
    jmp 1f
    nop
1:
# reload "stale" selectors
    movl $KDSEL,%eax
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%gs
    mov %ax,%ss
    mov %ax,%fs

# reload code selector by turning return into intersegmental return
    movl (%esp),%eax
    pushl %eax
    movl $KCSEL,4(%esp)
    lret

    .globl _lidt
_lidt:
    movl 4(%esp),%eax
    lidt (%eax)
    ret

    .globl _sys_vm86
_sys_vm86:
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    movl 20(%esp), %esi
    subl $(22*4), %esp
    movl %esp, %edi
    movl $22, %ecx
    cld
    rep movsl

    movl _g_task_running, %eax
    movl %edi, (%eax)

    addl $(3*4), %esp
    popal
    addl $(2*4), %esp
    iret

    .data
    .space 0x2000  # stack for the kernel
_tmp_stack:
    .globl _tmp_stack
