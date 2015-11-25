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

#include <inttypes.h>

static __inline uint8_t inportb(uint16_t port)
{
    register uint8_t val;
    __asm__ __volatile__ ("inb %%dx, %%al" : "=a" (val) : "d" (port));
    return( val );
}
static __inline uint16_t inportw(uint16_t port)
{
    register uint16_t val;
    __asm__ __volatile__ ("inw %%dx, %%ax" : "=a" (val) : "d" (port));
    return( val );
}
static __inline uint32_t inportl(uint16_t port)
{
    register uint32_t val;
    __asm__ __volatile__ ("inl %%dx, %%eax" : "=a" (val) : "d" (port));
    return( val );
}
static __inline void outportb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__ ("outb %%al, %%dx" : : "d" (port), "a" (val));
}

static __inline void outportw(uint16_t port, uint16_t val)
{
    __asm__ __volatile__ ("outw %%ax, %%dx" : : "d" (port), "a" (val));
}
static __inline void outportl(uint16_t port, uint32_t val)
{
    __asm__ __volatile__ ("outl %%eax, %%dx" : : "d" (port), "a" (val));
}

static __inline void insb(uint16_t port, void *addr, uint32_t count)
{
    __asm__ __volatile__ ("rep ; insb": "=D"(addr), "=c"(count)
            : "d"(port), "0"(addr), "1"(count));
}
static __inline void insw(uint16_t port, void *addr, uint32_t count)
{
    __asm__ __volatile__ ("rep ; insw": "=D"(addr), "=c"(count)
            : "d"(port), "0"(addr), "1"(count));
}
static __inline void insl(uint16_t port, void *addr, uint32_t count)
{
    __asm__ __volatile__ ("rep ; insl": "=D"(addr), "=c"(count)
            : "d"(port), "0"(addr), "1"(count));
}
static __inline void outsb(uint16_t port, void *addr, uint32_t count)
{
    __asm__ __volatile__ ("rep ; outsb": "=S"(addr), "=c"(count)
            : "d"(port), "0"(addr), "1"(count));
}
static __inline void outsw(uint16_t port, void *addr, uint32_t count)
{
    __asm__ __volatile__ ("rep ; outsw": "=S"(addr), "=c"(count)
            : "d"(port), "0"(addr), "1"(count));
}
static __inline void outsl(uint16_t port, void *addr, uint32_t count)
{
    __asm__ __volatile__ ("rep ; outsl": "=S"(addr), "=c"(count)
            : "d"(port), "0"(addr), "1"(count));
}

static __inline void cli()
{
    __asm__ __volatile__("cli");
}
static __inline void sti()
{
    __asm__ __volatile__("sti");
}

static __inline void
cpu_idle()
{
    __asm__ __volatile__("hlt");
}

#define save_flags_cli(flags)   \
    do {                        \
        __asm__ __volatile__(   \
                "pushfl\n\t"    \
                "popl %0\n\t"   \
                "cli\n\t"       \
                :"=g"(flags): );\
    } while(0)

#define restore_flags(flags)    \
    do {                        \
        __asm__ __volatile__(   \
                "pushl %0\n\t"  \
                "popfl\n\t"     \
                ::"g"(flags)    \
                :"memory" );    \
    } while(0)

static __inline void
invlpg(uint32_t addr)
{
    __asm__ __volatile__("invlpg %0" : : "m" (*(char *)addr) : "memory");
}

static __inline void
invltlb(void)
{
    uint32_t temp;
    __asm__ __volatile__("movl %%cr3, %0; movl %0, %%cr3" : "=r" (temp)
            : : "memory");
}

static __inline void
write_eflags(uint32_t ef)
{
    __asm__ __volatile__("pushl %0; popfl" : : "r" (ef));
}

static __inline uint32_t
read_eflags(void)
{
    uint32_t ef;

    __asm__ __volatile__("pushfl; popl %0" : "=r" (ef));
    return (ef);
}

#define PAGE_SHIFT  12
#define PGDR_SHIFT  22
#define PAGE_SIZE   (1<<PAGE_SHIFT)
#define PAGE_MASK   (PAGE_SIZE-1)
#define PAGE_TRUNCATE(x)  ((x)&(~PAGE_MASK))
#define PAGE_ROUNDUP(x)   (((x)+PAGE_MASK)&(~PAGE_MASK))

#define PTE_V   0x001 /* Valid */
#define PTE_W   0x002 /* Read/Write */
#define PTE_U   0x004 /* User/Supervisor */
#define PTE_A   0x020 /* Accessed */
#define PTE_M   0x040 /* Dirty */

#endif /*_CPU_H*/
