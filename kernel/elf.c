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
#ifdef __ELF__
#include <stdint.h>
#include <string.h>
#include "kernel.h"

typedef uint32_t        Elf32_Addr;
typedef uint16_t        Elf32_Half;
typedef uint32_t        Elf32_Off;
typedef int32_t         Elf32_Sword;
typedef uint32_t        Elf32_Word;

#define	EI_NIDENT	16

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* ident bytes */
#define	EI_MAG0		0	/* e_ident[] indexes */
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4	/* File class */
#define	EI_DATA		5	/* Data encoding */

#define	ELFMAG0		0x7f    /* EI_MAG */
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'

#define ELFCLASS32     1        /* 32-bit objects */

#define ELFDATA2LSB    1        /* Least significant byte encoding */

	Elf32_Half	e_type;			/* file type */
#define	ET_EXEC		2

	Elf32_Half	e_machine;		/* target machine */
#define	EM_386      3		/* Intel 80386 */
#define EM_ARM      40      /* ARM */

	Elf32_Word	e_version;		/* file version */
	Elf32_Addr	e_entry;		/* start address */
	Elf32_Off	e_phoff;		/* phdr file offset */
	Elf32_Off	e_shoff;		/* shdr file offset */
	Elf32_Word	e_flags;		/* file flags */
	Elf32_Half	e_ehsize;		/* sizeof ehdr */
	Elf32_Half	e_phentsize;		/* sizeof phdr */
	Elf32_Half	e_phnum;		/* number phdrs */
	Elf32_Half	e_shentsize;		/* sizeof shdr */
	Elf32_Half	e_shnum;		/* number shdrs */
	Elf32_Half	e_shstrndx;		/* shdr string index */
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	p_type;		/* entry type */
#define	PT_LOAD		1

	Elf32_Off	p_offset;	/* file offset */
	Elf32_Addr	p_vaddr;	/* virtual address */
	Elf32_Addr	p_paddr;	/* physical address */
	Elf32_Word	p_filesz;	/* file size */
	Elf32_Word	p_memsz;	/* memory size */
	Elf32_Word	p_flags;	/* entry flags */
#define	PF_R		0x4		/* p_flags */
#define	PF_W		0x2
#define	PF_X		0x1

	Elf32_Word	p_align;	/* memory/file alignment */
} Elf32_Phdr;

uint32_t load_aout(struct fs *fs, char *filename)
{
    int i, read;
    Elf32_Ehdr ehdr;
    uint32_t va, npages, prot;

    struct file *fp;
    if(0 != fs->open(fs, filename, O_RDONLY, &fp)) {
        printk("task #%d: failed to open %s\r\n",
                sys_task_getid(), filename);
        return 0;
    }

    read = fs->read(fp, (uint8_t *)&ehdr, sizeof(ehdr));
    if(
       (read != sizeof(ehdr)) ||
       (ehdr.e_ident[EI_MAG0] != ELFMAG0) ||
       (ehdr.e_ident[EI_MAG1] != ELFMAG1) ||
       (ehdr.e_ident[EI_MAG2] != ELFMAG2) ||
       (ehdr.e_ident[EI_MAG3] != ELFMAG3) ||
       (ehdr.e_ident[EI_CLASS] != ELFCLASS32) ||
       (ehdr.e_ident[EI_DATA] != ELFDATA2LSB) ||
       (ehdr.e_type != ET_EXEC) ||
#ifdef __arm__
       (ehdr.e_machine != EM_ARM)
#else
       (ehdr.e_machine != EM_386)
#endif
      ) {
        fs->close(fp);
        printk("task #%d: invalid executable file %s\r\n",
            sys_task_getid(), filename);
        return 0;
    }

    Elf32_Phdr *phdr = (Elf32_Phdr *)kmalloc(ehdr.e_phentsize*ehdr.e_phnum);
    fs->seek(fp, ehdr.e_phoff, SEEK_SET);
    read = fs->read(fp, (uint8_t *)phdr, ehdr.e_phentsize*ehdr.e_phnum);
    if(read != ehdr.e_phentsize*ehdr.e_phnum) {
        fs->close(fp);
        printk("task #%d: bad executable file %s\r\n",
            sys_task_getid(), filename);
        kfree(phdr);
        return 0;
    }

    for (i=0; i<ehdr.e_phnum; i++)         {
        switch(phdr[i].p_type) {
        case PT_LOAD:
            prot = VM_PROT_NONE;
            if(phdr[i].p_flags & PF_X)
                prot |= VM_PROT_EXEC;
            if(phdr[i].p_flags & PF_R)
                prot |= VM_PROT_READ;
            if(phdr[i].p_flags & PF_W)
                prot |= VM_PROT_WRITE;

            npages = PAGE_ROUNDUP((phdr[i].p_vaddr&PAGE_MASK)+phdr[i].p_memsz)/PAGE_SIZE;
            va = page_alloc_in_addr(PAGE_TRUNCATE(phdr[i].p_vaddr), npages, prot);
            if(va != PAGE_TRUNCATE(phdr[i].p_vaddr)) {
                fs->close(fp);
                printk("task #%d: Address 0x%08x of %d pages has already been used!\r\n",
                    sys_task_getid(),
                    PAGE_TRUNCATE(phdr[i].p_vaddr),
                    npages);
                kfree(phdr);
                return 0;
            }

            fs->seek(fp, phdr[i].p_offset, SEEK_SET);
            read = fs->read(fp, (uint8_t *)phdr[i].p_vaddr, phdr[i].p_filesz);
            if(read != phdr[i].p_filesz) {
                fs->close(fp);
                printk("task #%d: bad executable file %s\r\n",
                    sys_task_getid(), filename);
                kfree(phdr);
                return 0;
            }
            if(phdr[i].p_memsz > phdr[i].p_filesz)
                memset((void *)(phdr[i].p_vaddr+phdr[i].p_filesz),
                       0,
                       phdr[i].p_memsz-phdr[i].p_filesz);

            break;
        }
    }

    fs->close(fp);
    kfree(phdr);
    return ehdr.e_entry;
}
#endif /*__ELF__*/
