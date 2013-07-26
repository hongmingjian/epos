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
# $Id: crt0.S,v 1.1.1.1 2007/06/22 03:20:53 hmj Exp $
#
	.set SEL_CODE, 0x8
	.set SEL_DATA, 0x10
  .set HZ, 100
	.set TIME0_LATCH, 1193182/HZ

	.extern __bss_start__
	.extern _end

	.code16
	.globl _start
_start:
	cli
	cld
	movw $0x9000, %ax
	movw %ax, %ss
	movw $0xFFFE, %sp
  xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %es

#
# Detecting Memory and saving the system
# memory map to memory at 0x800
#
  .set SMAP, 0x800
e820:
  xorl %ebp, %ebp
  movw $(SMAP+4), %di
  xorl %ebx, %ebx 
e820.1:
  movl $0x0534D4150, %edx	# "SMAP"
  movl $0xe820, %eax
  movl $24, %ecx
  int  $0x15

  jc e820.2	
  movl $0x0534D4150, %edx	
  cmpl %eax, %edx
  jne e820.2
  incl %ebp
  testl %ebx, %ebx
  jz e820.2
  addw $24, %di
  jmp e820.1
e820.2:
  movl $SMAP, %eax
  movl %ebp, (%eax)

#
# Create IDT
#
	movw $0x30, %cx
	movw $idt, %di
	movw $intr, %si
again:
	lodsw
	stosw					# offset_0
	movw $SEL_CODE, %ax
	stosw					# selector
	xorb %al, %al
	stosb					# MBZ
	movb $0x8e, %al
	stosb					# flags
	xorw %ax, %ax
	stosw					# offset_16
	loop again

#
# Bring up the system.
#
	movw $TIME0_LATCH, %bx		# Set PIT
	callw setpit			#  latch

	movw $0x2820, %bx		# Set protected mode
	callw setpic			#  IRQ offsets
	lidt idtdesc	 		# Set IDT

	lgdt gdtdesc	 		# Set GDT
	movl %cr0, %eax			# Switch to protected
	orl $0x1, %eax			#  mode
	movl %eax, %cr0			#
	ljmp $SEL_CODE, $start32	# To 32-bit code

#
# Set latch for Timer 0 of 8253 PIT
#
setpit:
	movb $0x36, %al			# Select
	outb %al, $0x43			#  Timer 0
	movb %bl, %al			# Set latch
	outb %al, $0x40			#  LSB
	movb %bh, %al			#  and
	outb %al, $0x40			#  MSB
	ret

#
# Set IRQ offsets by reprogramming 8259A PICs.
#
setpic:
	movb $0x11,%al			# ICW1 to
	outb %al,$0x20			#  master,
	outb %al,$0xa0			#  slave
	movb %bl,%al			# ICW2 to
	outb %al,$0x21			#  master
	movb %bh,%al			# ICW2 to
	outb %al,$0xa1			#  slave
	movb $0x4,%al			# ICW3 to
	outb %al,$0x21			#  master
	movb $0x2,%al			# ICW3 to
	outb %al,$0xa1			#  slave
	movb $0x1,%al			# ICW4 to
	outb %al,$0x21			#  master,
	outb %al,$0xa1			#  slave
	ret				# To caller

	.code32
start32:
	xorl %ecx, %ecx			# Zero
	movb $SEL_DATA, %cl		# To 32-bit
	movw %cx, %ds
	movw %cx, %es
	movw %cx, %fs
	movw %cx, %gs
	movw %cx, %ss			#  stack
	movl $0xa0000, %esp

#
# Zero BSS
#
	xorl %eax, %eax
	movl $__bss_start__, %edi
	movl $_end, %ecx
	subl %edi, %ecx
	repnz stosb

#
# Initialize _intr_vector
#
	movl $0x10, %ecx
	movl $_intr_vector, %edi
	movl $_irq_default, %eax
	repnz stosl

#
# Mask all interrupts
#
	movb $0xfb, %al			# Mask
	outb %al, $0x21			#  master
	movb $0xff, %al			#  and
	outb %al, $0xa1			#  slave

#
# Enable interrupts
#
	sti

#
# Call main
#
	xorl %eax, %eax
	pushl %eax				# envirnp
	pushl %eax				# argv
	pushl %eax				# argc
	call _xmain
	addl $12, %esp

#
# Exit routine.
#
	.globl _exit
_exit:
	cli						# Disable interrupts
	movl %cr0,%eax			# Get CR0
	decl %eax				# Switch to
	movl %eax,%cr0			#  real mode
#
# reboot the machine
#
# For details, consult the following URL
#  http://www.faqs.org/faqs/msdos-programmer-faq/part4/section-16.html
#
_exit.1:		
	inb  $0x64, %al
	testb $0x2, %al
	jnz _exit.1
	movb $0xfe, %al
	outb %al, $0x64

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
	movl $1000, %ecx
_outportb.1:
  loop _outportb.1
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
  movl $1000, %ecx
_outportw.1:	
  loop _outportw.1
  ret

divide_error:
	pushl	$0
	pushl	$0
	jmp		exception
debug:
	pushl	$0
	pushl	$1
	jmp		exception
nmi:
	pushl   $0
	pushl   $2
	jmp		exception
breakpoint:
	pushl   $0
	pushl   $3
	jmp		exception
overflow:
	pushl	$0
	pushl	$4
	jmp		exception
bounds_check:
	pushl	$0
	pushl	$5
	jmp		exception
inval_opcode:
	pushl	$0
	pushl	$6
	jmp		exception
copr_not_avail:
	pushl	$0
	pushl	$7
	jmp		exception
double_fault:
	pushl	$8
	jmp		exception
copr_seg_overrun:
	pushl	$0
	pushl	$9
	jmp		exception
inval_tss:
	pushl	$10
	jmp		exception
segment_not_present:
	pushl	$11
	jmp		exception
stack_exception:
	pushl	$12
	jmp		exception
general_protection:
	pushl	$13
	jmp		exception
page_fault:
	pushl	$14
	jmp		exception
intel_reserved:
	pushl	$0
	pushl	$15
	jmp		exception
copr_error:
	pushl	$0
	pushl	$16
	jmp		exception
alignment_check:
	pushl	$17
	jmp		exception
machine_check:
	pushl	$0
	pushl	$18
	jmp		exception
simd_fp_exception:
	pushl	$0
	pushl	$19
	jmp		exception

exception:
	call _abort
  call _exit
	
	/* SHOULD NOT REACHED */

_irq_default:
	ret

	.p2align 4
	.globl _intr_vector
_intr_vector:	.fill 0x10, 4, 0

#define hwint_master(irq)				\
	pushal;					\
	pushl $irq;				\
	movl (_intr_vector + 4 * irq), %eax;	\
	call *%eax;				\
	popl  %eax;				\
	movb  $0x20, %al;			\
	outb  %al, $0x20;			\
	popal;					\
	iret
hwint00:
	hwint_master(0)
hwint01:
	hwint_master(1)
hwint02:
	hwint_master(2)
hwint03:
	hwint_master(3)
hwint04:
	hwint_master(4)
hwint05:
	hwint_master(5)
hwint06:
	hwint_master(6)
hwint07:
	hwint_master(7)

#define hwint_slave(irq)				\
	pushal;					\
	pushl $irq;				\
	movl (_intr_vector + 4 * irq), %eax;	\
	call *%eax;				\
	popl  %eax;				\
	movb  $0x20, %al;			\
	outb  %al, $0x20;			\
	outb  %al, $0xa0;			\
	popal;					\
	iret
hwint08:
	hwint_slave(8)
hwint09:
	hwint_slave(9)
hwint10:
	hwint_slave(10)
hwint11:
	hwint_slave(11)
hwint12:
	hwint_slave(12)
hwint13:
	hwint_slave(13)
hwint14:
	hwint_slave(14)
hwint15:
	hwint_slave(15)

	.p2align 4
intr:
	.word divide_error,   debug,               nmi,             breakpoint
	.word overflow,       bounds_check,        inval_opcode,    copr_not_avail
  .word double_fault,   copr_seg_overrun
	.word inval_tss,      segment_not_present, stack_exception, general_protection
	.word page_fault,     intel_reserved,      copr_error,      alignment_check
	.word machine_check,  simd_fp_exception,   intel_reserved,  intel_reserved
	.word intel_reserved, intel_reserved,      intel_reserved,  intel_reserved
	.word intel_reserved, intel_reserved,      intel_reserved,  intel_reserved
	.word intel_reserved, intel_reserved
	.word hwint00,        hwint01,             hwint02,         hwint03
	.word hwint04,        hwint05,             hwint06,         hwint07
	.word hwint08,        hwint09,             hwint10,         hwint11
	.word hwint12,        hwint13,             hwint14,         hwint15

#
# Global descriptor table.
#
	.p2align 4
gdt:
  .word 0x0,0x0,0x0,0x0		# Null entry
	.word 0xffff,0x0,0x9a00,0xcf	# SEL_CODE
	.word 0xffff,0x0,0x9200,0xcf	# SEL_DATA
gdt.1:
gdtdesc:
  .word gdt.1-gdt-1, gdt, 0

#
# Interrupt descriptor table.
#
	.p2align 4
idt:
  .fill 0x30, 8, 0
idtdesc:
  .word (0x30*8)-1, idt, 0

#
# End of file
#
