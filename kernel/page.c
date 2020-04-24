/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2015 Hong MingJian<hongmingjian@gmail.com>
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
#include <stddef.h>
#include <string.h>
#include "kernel.h"

static struct vmzone km0;
static struct vmzone *kvmzone;

static struct vmzone *uvmzone;

void init_vmspace(uint32_t brk)
{
    km0.base = USER_MAX_ADDR;
    km0.limit = brk - km0.base;
    km0.prot = VM_PROT_ALL;
    km0.fp = NULL;
    km0.offset = 0;
    km0.flags = 0;
    km0.next = NULL;
    kvmzone = &km0;

    uvmzone = NULL;
}

/**
 * 在指定的虚拟地址va，分配npages个连续页面
 */
struct vmzone *page_alloc_in_addr(uint32_t va, int npages, int prot,
                                  int flags, struct file *fp, off_t offset)
{
    uint32_t flagx;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return NULL;
    if(va & PAGE_MASK)
        return NULL;

    /*必须在[USER_MIN_ADDR, KERN_MAX_ADDR)之间*/
    if(va <  USER_MIN_ADDR ||
       va >= KERN_MAX_ADDR ||
       va + size > KERN_MAX_ADDR)
        return NULL;

    save_flags_cli(flagx);

    struct vmzone *p = kvmzone, *q = NULL;
    if(va < USER_MAX_ADDR) {
        /*不能跨越用户空间和内核空间的边界*/
        if(va + size > USER_MAX_ADDR) {
            restore_flags(flagx);
            return NULL;
        }
        p = uvmzone;
    }

    for(; p != NULL; q = p, p = p->next) {
        if(va >= p->base &&
           va <  p->base + p->limit) {
            restore_flags(flagx);
            return NULL;
        }
        if(va < p->base) {
            if(va + size > p->base) {
                restore_flags(flagx);
                return NULL;
            }
            break;
        }
    }

    struct vmzone *z = (struct vmzone *)kmalloc(sizeof(struct vmzone));
    z->base = va;
    z->limit = size;
    z->prot = prot;
    z->fp = fp;
    if(z->fp)
		z->fp->refcnt++;
    z->offset = offset;
    z->flags = flags;

    if(q == NULL) {
        z->next = p;
        uvmzone = z;
    } else {
        z->next = q->next;
        q->next = z;
    }

    restore_flags(flagx);
    return z;
}

/**
 * 在地址空间中分配npages个连续页面，返回页面所在的起始地址
 * user是0表示在内核空间中分配，否则在用户空间中分配
 */
struct vmzone *page_alloc(int npages, int prot, int user,
                          int flags, struct file *fp, off_t offset)
{
    uint32_t flagx;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return NULL;

    save_flags_cli(flagx);

    struct vmzone *p = kvmzone, *q = NULL;
    if(user) {
        p = uvmzone;
    }

    uint32_t va;
    if(p == NULL)
        va = USER_MIN_ADDR;
    else {
        va = p->base+p->limit;
        q = p, p = p->next;
    }

    for(; p != NULL; q = p, p = p->next) {
        if(va + size <= p->base)
            break;
        va = p->base+p->limit;
    }

    if(user) {
        if(va >= USER_MAX_ADDR ||
           va + size > USER_MAX_ADDR) {
            restore_flags(flagx);
            return NULL;
        }
    } else {
        if(va >= KERN_MAX_ADDR ||
           va + size > KERN_MAX_ADDR) {
            restore_flags(flagx);
            return NULL;
        }
    }

    struct vmzone *z = (struct vmzone *)kmalloc(sizeof(struct vmzone));
    z->base = va;
    z->limit = size;
    z->prot = prot;
    z->fp = fp;
    if(z->fp)
		z->fp->refcnt++;
    z->offset = offset;
    z->flags = flags;

    if(q == NULL) {
        z->next = p;
        uvmzone = z;
    } else {
        z->next = q->next;
        q->next = z;
    }

    restore_flags(flagx);
    return z;
}

/**
 * 释放page_alloc/page_alloc_in_addr所分配的页面
 *
 */
int page_free(uint32_t va, int npages)
{
    uint32_t flags;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return -1;

    /*不能释放km0*/
    if(va == USER_MAX_ADDR)
        return -1;

    save_flags_cli(flags);

    struct vmzone *p = kvmzone, *q = NULL;
    if(va < USER_MAX_ADDR) {
        p = uvmzone;
    }
    for(; p != NULL; q = p, p = p->next) {
		if((va   == p->base +((p->flags&MAP_STACK)?PAGE_SIZE:0)) &&
		   (size == p->limit-((p->flags&MAP_STACK)?PAGE_SIZE:0))) {
            if(q == NULL) {
                uvmzone = p->next;
            } else {
                q->next = p->next;
            }
            restore_flags(flags);

            if(p->fp) {
				if(p->flags & MAP_SHARED) {
					uint32_t pte;
					int i;
					for(i = 0; i < p->limit; i+=PAGE_SIZE) {
						pte = *vtopte(va+i);
						if(pte & L2E_V) {
							if(p->fp->fs->seek(p->fp, p->offset+i, SEEK_SET) >= 0)
								p->fp->fs->write(p->fp, (void *)(va+i), PAGE_SIZE);
						}
					}
				}

				p->fp->refcnt--;
				if(p->fp->refcnt == 0) {
					p->fp->fs->close(p->fp);
				}
            }

            kfree(p);
            return 0;
        }
    }

    restore_flags(flags);
    return -1;
}

/**
 * 返回va所在区域
 */
struct vmzone *page_zone(uint32_t va)
{
    uint32_t flags;
    save_flags_cli(flags);

    struct vmzone *p = kvmzone;
    if(va < USER_MAX_ADDR) {
        p = uvmzone;
    }
    for(; p != NULL; p = p->next) {
        if(va >= p->base &&
           va <  p->base+p->limit) {
            restore_flags(flags);
            return p;
        }
    }

    restore_flags(flags);
    return NULL;
}

/**
 * 把从vaddr开始的虚拟地址，映射到paddr开始的物理地址。
 * 共映射npages页面，把PTE的标志位设为flags
 */
void page_map(uint32_t vaddr, uint32_t paddr, uint32_t npages, uint32_t flags)
{
    for (; npages > 0; npages--){
        *vtopte(vaddr) = paddr | flags;
        invlpg(vaddr);
        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }
}

/**
 * 取消page_map的所做的映射
 */
void page_unmap(uint32_t vaddr, uint32_t npages)
{
    for (; npages > 0; npages--){
        *vtopte(vaddr) = 0;
        invlpg(vaddr);
        vaddr += PAGE_SIZE;
    }
}

