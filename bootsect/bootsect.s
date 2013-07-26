##
## bootsect.s - 512 bytes bootblock for floppy disk
##
## PURPOSE This bootblock loads the file `eposldr.bin' from the root
##         of FAT file system into the 0800:0000, then transfers the
##         control to there with interrupt disabled, nothing more.
##
## Copyright (C) 2005 Hong MingJian
## All rights reserved.
##
## Redistribution and use in source and binary forms are freely
## permitted provided that the above copyright notice and this
## paragraph and the following disclaimer are duplicated in all
## such forms.
##
## This software is provided "AS IS" and without any express or
## implied warranties, including, without limitation, the implied
## warranties of merchantability and fitness for a particular
## purpose.
##
## $Id: bootsect.s,v 1.1.1.1 2006/05/20 01:33:18 hmj Exp $
##
		.code16
		.text

		.set    LOAD, 0x07C0 # segment
		.set    RUN, 0x9000  # segment

		.global _start
_start:
        jmp go
        nop

bsOEMName:      .ascii  "MSWIN4.1"

#
# See this URL for FAT specification
#  http://www.microsoft.com/whdc/system/platform/firmware/fatgen.mspx
#
# BIOS Parameter Block(BPB)
#
bpbBytsPerSec:  .word   0x200           # 512
bpbSecPerClus:  .byte   0x1             # 1
bpbRsvdSecCnt:  .word   0x1             # 1
bpbNumFATs:     .byte   0x2             # 2
bpbRootEntCnt:  .word   0xE0            # 224
bpbTotSec16:    .word   0xB40           # 2880(UNUSED)
bpbMedia:       .byte   0xF0            # floppy(UNUSED)
bpbFATSz16:     .word   0x9             # 9
bpbSecPerTrk:   .word   0x12            # 18
bpbNumHeads:    .word   0x2             # 2
bpbHiddSec:     .long   0x0             # 0(UNUSED)
bpbTotSec32:    .long   0x0             # 0(UNUSED)
#
# Extended BPB for FAT12 or FAT16
#
bsDrvNum:       .byte   0x0
bsReserved1:    .byte   0x0             # 0(UNUSED)
bsBootSig:      .byte   0x29            # 41(UNUSED)
bsVolID:        .long   0x19770802		# (UNUSED)
bsVolLab:       .ascii  "NO NAME    "	# (UNUSED)
bsFilSysType:   .ascii  "FAT12   "		# (UNUSED)

go:
        cli
        cld
#
# relocate from LOAD to RUN
#
        movw $LOAD, %ax
        movw %ax, %ds

        movw $RUN, %ax
        movw %ax, %es

        xorw %si, %si
        xorw %di, %di
        movw $0x100, %cx
        rep
        movsw
#
# And jump to it
#
        ljmp $RUN, $main
main:
        movw %cs, %ax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %ss
        movw $0xFFFE, %sp
#
# calculate the paragraphs per cluster
# 1 paragraphs = 16 bytes
#
        xorw %ax, %ax
        movb bpbSecPerClus, %al
        mulw bpbBytsPerSec
        shrw $0x4, %ax
        movw %ax, paras_per_cluster
#
#  read root dir to the memory
#
        movw $0x20, %ax
        mulw bpbRootEntCnt
        divw bpbBytsPerSec
        movw %ax, %cx

        xorw %ax, %ax
        movb bpbNumFATs, %al
        mulw bpbFATSz16
        addw bpbRsvdSecCnt, %ax

        movw %ax, datasector
        addw %cx, datasector

        movw $buffer, %bx
        call sread
#
# find the image file to be loaded
#
        movw $buffer, %di # root dir
        movw $image, %si  # file name
        call findfile

        pushw 0x1A(%di)   # save 1st cluster of the file just found
#
# load the FAT to memory
#
        movw $buffer, %bx
        movw bpbRsvdSecCnt, %ax
        movw bpbFATSz16, %cx
        call sread
#
# load the image file to 0000:0500
#
        popw %ax                # 1st cluster
        movw $buffer, %di       #  FAT
        movw $0x800, %bx
        movw %bx, %es           #  start segment
        call readfile

        call killmotor
        call seta20
#
# disable interrupt
#
        cli
#
# to mode 13h (320x200, 256 colors)
#
#		movw $0x13, %ax
#		int $0x10

#
# jump
#
        ljmp $0x800, $0x0
#
#------------------------------------------------
#
#                Helper routines
#
#------------------------------------------------
#
# Inputs:
#         %di - root dir entry
#         %si - file name
# Outputs:
#         %di - dir entry of the found file
# Modifies:
#         %cx
#
findfile:
        movw bpbRootEntCnt, %cx
findfile.1:
        testb $0x18, 0xB(%di)
        jnz findfile.2                   # skip dir and volume
        pushw %di
        pushw %si
        pushw %cx
        movw $0xB, %cx					 # 8.3 naming convention
        rep
        cmpsb
        popw %cx
        popw %si
        popw %di
        je findfile.3
findfile.2:
        addw $0x20, %di
        decw %cx
        jnz findfile.1
        jmp die
findfile.3:
        ret
#
# readfile - load the file given by its 1st cluster address
#            and the FAT
# Inputs:
#         %ax - 1st cluster of the file
#         %di - points to the FAT
#         %es - start segment of the buffer
# Outputs:
#         %es - end segment
# Modifies:
#         %bx, %ax
#
readfile:
        movw %es, %bx
readfile.1:
        addw paras_per_cluster, %bx
        pushw %bx
        xorw %bx, %bx
        call cread
        call cnext
        popw %bx
        movw %bx, %es
        cmpw $0xFFF, %ax
        jne readfile.1
        ret
#
# cread - read a cluster
# Inputs:
#         %ax - address of the cluster
#         %es:%bx - the buffer
# Ouputs:
#         None
# Modifies:
#         %cx
#
cread:
      pushw %ax
      call cluster2lba
      xorw %cx, %cx
      movb bpbSecPerClus, %cl
      call sread
      popw %ax
      ret
#
# cnext - retrieve the next cluster from the FAT
# Inputs:
#         %ax - the current cluster
#         %di - point to the FAT
# Outputs:
#         %ax - the next cluster
# Modifies:
#         %bx
#
cnext:
      movw %ax, %bx
      shrw $0x1, %bx
      addw %ax, %bx
      addw %di, %bx
      pushw (%bx)
      testw $0x1, %ax
      popw %ax
      jz cnext.1
      shrw $0x4, %ax      # odd
      jmp cnext.2
cnext.1:                  # even
      andw $0xFFF, %ax
cnext.2:
      ret
#
# sread - read several contiguous sectors
# Inputs:
#         %ax - starting sector
#         %cx - # of sectors
#         %es:%bx - buffer
# Outputs:
#         None
# Modifies:
#         %cx, %ax, %bp, %bx
#
sread:
      movw $0x3, %bp
sread.1:
      pushw %ax
      pushw %cx
      pushw %dx
      call lba2chs
      movw $0x201, %ax
      movb bsDrvNum, %dl
      int $0x13
      jnc sread.2
      xorw %ax, %ax      # reset
      int $0x13          #  floppy
      popw %dx
      popw %cx
      popw %ax
      decw %bp
      jnz sread.1
      jmp die
sread.2:
      popw %dx
      popw %cx
      popw %ax
      addw bpbBytsPerSec, %bx
      incw %ax
      decw %cx
      jnz sread
      ret
#
# cluster2lba - convert cluster address to LBA address
# Inputs:
#         %ax - the cluster address
#         datasector
# Outputs:
#         %ax - the converted LBA address
# Modifies:
#         %cx, %dx
#
cluster2lba:
      subw $0x2, %ax
      xorw %cx, %cx
      movb bpbSecPerClus, %cl
      mulw %cx
      addw datasector, %ax
      ret
#
# lba2chs - Convert Logical Block Address to CHS address
# Inputs:
#         %ax - LBA address
# Ouputs:
#         %ch - Cylinder
#         %dh - Head
#         %cl - Sector
# Modifies:
#         None
#
lba2chs:
      pushw %ax
      xorw %dx, %dx
      divw bpbSecPerTrk
      incb %dl
      movb %dl, %cl
      xorw %dx, %dx
      divw bpbNumHeads
      movb %dl, %dh
      movb %al, %ch
      popw %ax
      ret
#
# killmotor - turn off floppy dirve light
# Inputs:
#         None
# Outputs:
#         None
# Modifies:
#         %cx
#
killmotor:
        movw $0x64, %cx
killmotor.1:
        int $0x8
        decw %cx
        jnz killmotor.1
        ret
#
# seta20 - Enable A20 so we can access memory above 1 meg.
# Inputs:
#         None
# Outputs:
#         None
# Modifies:
#         %ax
#
# Note:
#         This piece of code comes from FreeBSD
#
seta20:
      cli                  # Disable interrupts
seta20.1:
      inb $0x64,%al        # Get status
      testb $0x2,%al       # Busy?
      jnz seta20.1         # Yes
      movb $0xD1,%al       # Command: Write
      outb %al,$0x64       #  output port
seta20.2:
      inb $0x64,%al        # Get status
      testb $0x2,%al       # Busy?
      jnz seta20.2         # Yes
      movb $0xDF,%al       # Enable
      outb %al,$0x60       #  A20
      sti                  # Enable interrupts
      ret                  # To callwer
#
# die
#
die:  jmp die

#
# data
#
paras_per_cluster: .word 0x0
datasector:     .word 0x0
image:          .ascii "EPOSLDR BIN"

		.org 510
		.word 0xAA55
#
# The buffer
#
buffer:
