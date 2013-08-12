#
# Copyright (C) 2005, 2013 Hong MingJian
# All rights reserved.
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
  .extern __bss_start
  .extern _end
  .extern _intr_vector
  .extern _g_resched
  .extern _schedule
  .extern _g_task_running
  .extern _syscall


#define KERNBASE 0xC0000000

#define KDSEL 0x10
#define KCSEL 0x8

#define R(foo) ((foo)-KERNBASE)

  .code32
  .globl _entry
_entry:
  pushl %ebp
  movl %esp, %ebp

	pushl	$0x2
	popfl

	movw	%ds, %ax
	movw	%ax, %fs
	movw	%ax, %gs
  movl $R(_kern_stack), %esp

#
# Zero BSS
#
  xorl %eax, %eax
  movl $R(__bss_start), %edi
  movl $R(_end), %ecx
  subl %edi, %ecx
  repnz stosb

  call _cstart

  hlt

  .global	_inportb
_inportb:
  movw 4(%esp), %dx
  inb	%dx, %al
  movzx %al, %eax
  ret

  .global	_outportb
_outportb:
  movw 4(%esp), %dx
  movb 8(%esp), %al
  outb %al, %dx
	ret

  .global	_inportw
_inportw:
  movw 4(%esp), %dx
  inw	%dx, %ax
  movzx %ax, %eax
  ret

  .global	_outportw
_outportw:
  movw 4(%esp), %dx
  movw 8(%esp), %ax
  outw %ax, %dx
  ret

  .globl _exception_divide_error
_exception_divide_error:
	pushl	$0
	pushl	$0
	jmp		exception

  .globl _exception_debug
_exception_debug:
	pushl	$0
	pushl	$1
	jmp		exception

  .globl _exception_nmi
_exception_nmi:
	pushl   $0
	pushl   $2
	jmp		exception

  .globl _exception_breakpoint
_exception_breakpoint:
	pushl   $0
	pushl   $3
	jmp		exception

  .globl _exception_overflow
_exception_overflow:
	pushl	$0
	pushl	$4
	jmp		exception

  .globl _exception_bounds_check
_exception_bounds_check:
	pushl	$0
	pushl	$5
	jmp		exception

  .globl _exception_inval_opcode
_exception_inval_opcode:
	pushl	$0
	pushl	$6
	jmp		exception

  .globl _exception_copr_not_avail
_exception_copr_not_avail:
	pushl	$0
	pushl	$7
	jmp		exception

  .globl _exception_double_fault
_exception_double_fault:
	pushl	$8
	jmp		exception

  .globl _exception_copr_seg_overrun
_exception_copr_seg_overrun:
	pushl	$0
	pushl	$9
	jmp		exception

  .globl _exception_inval_tss
_exception_inval_tss:
	pushl	$10
	jmp		exception

  .globl _exception_segment_not_present
_exception_segment_not_present:
	pushl	$11
	jmp		exception

  .globl _exception_stack_exception
_exception_stack_exception:
	pushl	$12
	jmp		exception

  .globl _exception_general_protection
_exception_general_protection:
	pushl	$13
	jmp		exception

  .globl _exception_page_fault
_exception_page_fault:
	pushl	$14
	jmp		exception

  .globl _exception_intel_reserved
_exception_intel_reserved:
	pushl	$0
	pushl	$15
	jmp		exception

  .globl _exception_copr_error
_exception_copr_error:
	pushl	$0
	pushl	$16
	jmp		exception

  .globl _exception_alignment_check
_exception_alignment_check:
	pushl	$17
	jmp		exception

  .globl _exception_machine_check
_exception_machine_check:
	pushl	$0
	pushl	$18
	jmp		exception

  .globl _exception_simd_fp_exception
_exception_simd_fp_exception:
	pushl	$0
	pushl	$19
	jmp		exception

#
# XXX - Should we handle the exception
#       in the context of the running task?
#
exception:
	pushal
  pushl %ds
  pushl %es
  pushl %fs
  movl $KDSEL, %eax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  pushl %esp
	call _exception
  addl $4, %esp
  popl %fs
  popl %es
  popl %ds
  popal
  addl $8, %esp # discard trapno and code
  iret
	
  .globl _int0x80_syscall
_int0x80_syscall:
  subl $8, %esp # fake trapno and code
  pushal 
  pushl %ds
  pushl %es
  pushl %fs
  movl $KDSEL, %eax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs

  movl _g_task_running, %eax
  movl %esp, %esi
  movl %eax, %edi
  subl $(18*4), %edi
  movl $18, %ecx
  movl %edi, %edx
  cld
  rep
  movsd
  movl %edx, %esp

  pushl %esp
  sti
  call _syscall
  cli
  addl $4, %esp

  .globl _ret_from_syscall
_ret_from_syscall:
  popl %fs
  popl %es
  popl %ds
	popal
  addl $8, %esp
	iret

#define	ENABLE_ICU1 \
	movb  $0x20, %al; \
	outb  %al, $0x20

#define	ENABLE_ICU1_AND_2 \
  ENABLE_ICU1; \
	outb  %al, $0xa0

#define hwint(irq, enable_icus) \
  subl $8, %esp; \
  pushal; \
  pushl %ds; \
  pushl %es; \
  pushl %fs; \
  ; \
  movl $KDSEL, %eax; \
  movw %ax, %ds; \
  movw %ax, %es; \
  movw %ax, %fs; \
  ; \
  movl _g_task_running, %eax; \
  cmpl $0, %eax; \
  je 1f; \
  movl 56(%esp), %ebx; \
  andl $3, %ebx; \
  jz 1f; \
  ; \
  movl %esp, %esi; \
  movl %eax, %edi; \
  subl $(18*4), %edi; \
  movl $18, %ecx; \
  movl %edi, %edx; \
  ; \
  cld; \
  rep; \
  movsd; \
  ; \
  movl %edx, %esp; \
1:; \
  pushl %esp; \
	pushl $irq; \
	movl (_intr_vector + 4 * irq), %eax; \
	call *%eax; \
	addl $8, %esp; \
  ; \
	enable_icus; \
  ; \
	cmpl $0, _g_resched; \
	je 2f; \
	call _schedule; \
2:; \
  jmp _ret_from_syscall

  .globl _hwint00
_hwint00:
	hwint(0, ENABLE_ICU1)

  .globl _hwint01
_hwint01:
  hwint(1, ENABLE_ICU1)

  .globl _hwint02
_hwint02:
	hwint(2, ENABLE_ICU1)

  .globl _hwint03
_hwint03:
	hwint(3, ENABLE_ICU1)
  
  .globl _hwint04
_hwint04:
	hwint(4, ENABLE_ICU1)

  .globl _hwint05
_hwint05:
	hwint(5, ENABLE_ICU1)

  .globl _hwint06
_hwint06:
	hwint(6, ENABLE_ICU1)

  .globl _hwint07
_hwint07:
	hwint(7, ENABLE_ICU1)

  .globl _hwint08
_hwint08:
	hwint(8, ENABLE_ICU1_AND_2)

  .globl _hwint09
_hwint09:
	hwint(9, ENABLE_ICU1_AND_2)

  .globl _hwint10
_hwint10:
	hwint(10, ENABLE_ICU1_AND_2)

  .globl _hwint11
_hwint11:
	hwint(11, ENABLE_ICU1_AND_2)

  .globl _hwint12
_hwint12:
	hwint(12, ENABLE_ICU1_AND_2)

  .globl _hwint13
_hwint13:
	hwint(13, ENABLE_ICU1_AND_2)

  .globl _hwint14
_hwint14:
	hwint(14, ENABLE_ICU1_AND_2)

  .globl _hwint15
_hwint15:
	hwint(15, ENABLE_ICU1_AND_2)

  .globl _lgdt
_lgdt:
	movl	4(%esp),%eax
	lgdt	(%eax)

	# flush the prefetch q
	jmp	1f
	nop
1:
	# reload "stale" selectors
	movl	$KDSEL,%eax
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%gs
	mov	%ax,%ss
	mov	%ax,%fs

	# reload code selector by turning return into intersegmental return
	movl	(%esp),%eax
	pushl	%eax
	movl	$KCSEL,4(%esp)
	lret

  .globl _lidt
_lidt:
	movl	4(%esp),%eax
	lidt	(%eax)
	ret

  .data
	.space	0x2000		# stack for the kernel
_kern_stack:
  .globl	_kern_stack
