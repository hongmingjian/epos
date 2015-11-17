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
#include "kernel.h"
#include "dosfs.h"

struct pvmap {
    uint32_t paddr;
    struct _vaddr {
        uint32_t pgdir;
        uint32_t vaddr;
        struct _vaddr *next;
    } vaddrs;
    struct pvmap *next;
};

struct vsmap {
    uint32_t pgdir;
    uint32_t vaddr;
    uint32_t length;
    uint32_t fd;
    uint32_t offset;
    struct vsmap *next;
};

struct vmzone {
    uint32_t base;
    uint32_t limit;
    uint32_t protect;
    struct vmzone *next;
};

static struct vmzone km0;
static struct vmzone *kvmzone;

static struct vmzone *uvmzone;

#if 1
void init_vmspace(uint32_t brk)
{
    km0.base = USER_MAX_ADDR;
    km0.limit = brk - km0.base;
    km0.next = NULL;
    kvmzone = &km0;

    uvmzone = NULL;
}

/**
 * 在指定的虚拟地址va，分配npages个连续页面
 * 失败返回SIZE_MAX，成功返回va
 */
uint32_t page_alloc_in_addr(uint32_t va, int npages)
{
    uint32_t flags;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return SIZE_MAX;
    if(va & PAGE_MASK)
        return SIZE_MAX;
    if(va < USER_MIN_ADDR)
        return SIZE_MAX;

    save_flags_cli(flags);

    struct vmzone *p = kvmzone, *q = NULL;
    if(va < USER_MAX_ADDR) {
        /*不能跨越用户空间和内核空间的边界*/
        if(va + size > USER_MAX_ADDR) {
            restore_flags(flags);
            return SIZE_MAX;
        }
        p = uvmzone;
    }

    for(; p != NULL; q = p, p = p->next) {
        if(va >= p->base &&
           va <  p->base + p->limit) {
            restore_flags(flags);
            return SIZE_MAX;
        }
        if(va < p->base) {
            if(va + size > p->base) {
                restore_flags(flags);
                return SIZE_MAX;
            }
            break;
        }
    }

    struct vmzone *x = (struct vmzone *)kmalloc(sizeof(struct vmzone));
    x->base = va;
    x->limit = size;

    if(q == NULL) {
        x->next = p;
        uvmzone = x;
    } else {
        x->next = q->next;
        q->next = x;
    }

    restore_flags(flags);
    return va;
}

/**
 * 在地址空间中分配npages个连续页面，返回页面所在的起始地址
 * user是0表示在内核空间中分配，否则在用户空间中分配
 */
uint32_t page_alloc(int npages, uint32_t user)
{
    uint32_t flags;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return SIZE_MAX;

    save_flags_cli(flags);

    struct vmzone *p = kvmzone, *q = NULL;
    uint32_t va = p->base+p->limit;
    q = p, p = p->next;
    if(user) {
        p = uvmzone;
        if(p == NULL)
            va = USER_MIN_ADDR;
        else {
            va = p->base+p->limit;
            q = p, p = p->next;
        }
    }

    for(; p != NULL; q = p, p = p->next) {
        if(va < p->base &&
           va + size <= p->base)
            break;
        va = p->base+p->limit;
    }

    if(user) {
        if(va >= USER_MAX_ADDR ||
           va + size > USER_MAX_ADDR) {
            restore_flags(flags);
            return SIZE_MAX;
        }
    } else {
        if(va >= KERN_MAX_ADDR ||
           va + size > KERN_MAX_ADDR) {
            restore_flags(flags);
            return SIZE_MAX;
        }
    }

    struct vmzone *x = (struct vmzone *)kmalloc(sizeof(struct vmzone));
    x->base = va;
    x->limit = size;

    if(q == NULL) {
        x->next = p;
        uvmzone = x;
    } else {
        x->next = q->next;
        q->next = x;
    }

    restore_flags(flags);

    return va;
}

/**
 * 释放page_alloc所分配的页面
 *
 */
int page_free(uint32_t va, int npages)
{
    uint32_t flags;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return -1;
    if(va == USER_MAX_ADDR)
        return -1;

    save_flags_cli(flags);

    struct vmzone *p = kvmzone, *q = NULL;
    if(va < USER_MAX_ADDR) {
        p = uvmzone;
    }
    for(; p != NULL; q = p, p = p->next) {
        if(va == p->base && size == p->limit) {
            if(q == NULL) {
                uvmzone = p->next;
            } else {
                q->next = p->next;
            }
            kfree(p);
            restore_flags(flags);
            return 0;
        }
    }

    restore_flags(flags);
    return -1;
}

/**
 * 检查地址是否合法
 * 合法返回1，否则返回0
 */
int page_check(uint32_t va)
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
            return 1;
        }
    }

    restore_flags(flags);
    return 0;

}
/////////////////////////////////////////////////////////////////////////
#else
static struct vmzone um0;
void init_vmspace()
{
    km0.base = PAGE_ROUNDUP( (uint32_t)(&end) );
    km0.limit = KERN_MAX_ADDR-km0.base;
    km0.next = NULL;
    kvmzone = &km0;

    um0.base = USER_MIN_ADDR;
    um0.limit = USER_MAX_ADDR-um0.base;
    um0.next = NULL;
    uvmzone = &um0;
}

/**
 * 在指定的虚拟地址va，分配npages个连续页面
 * 失败返回SIZE_MAX，成功返回va
 */
uint32_t page_alloc_in_addr(uint32_t va, int npages)
{
    uint32_t flags;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return SIZE_MAX;

    struct vmzone *p = kvmzone;
    if(va < USER_MAX_ADDR)
        p = uvmzone;

    save_flags_cli(flags);
    for(; p != NULL; p = p->next) {
        if(va >= p->base &&
           va <  p->base + p->limit &&
           va + size >  p->base &&
           va + size <= p->base+p->limit) {
            break;
       }
   }

   if(p != NULL) {
       if(va == p->base) {
           p->base += size;
           p->limit -= size;
           restore_flags(flags);
           return va;
       }

       if(va + size == p->base + p->limit) {
           p->limit -= size;
           restore_flags(flags);
           return va;
       }

       struct vmzone *x = (struct vmzone *)kmalloc(sizeof(struct vmzone));
       x->base = va + size;
       x->limit = p->limit-(x->base-p->base);
       x->next = p->next;

       p->limit = (va - p->base);
       p->next = x;

       restore_flags(flags);
       return va;
   }

   restore_flags(flags);
   return SIZE_MAX;
}

/**
 * 在地址空间中分配npages个连续页面，返回页面所在的起始地址
 * user是0表示在内核空间中分配，否则在用户空间中分配
 */
uint32_t page_alloc(int npages, uint32_t user)
{
    uint32_t flags;
    uint32_t va = SIZE_MAX;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return va;

    struct vmzone *q = NULL, *p = kvmzone;
    if(user)
       p = uvmzone;

    save_flags_cli(flags);
    while(p != NULL) {
        if(p->limit >= size) {
            va = p->base;
            p->base += size;
            p->limit -= size;
            if(p->limit == 0) {
                if(p != &km0 && p != &um0) {
                    if(q == NULL) {
                        if(user)
                            uvmzone = p->next;
                        else
                            kvmzone = p->next;
                    } else
                        q->next = p->next;
                    kfree(p);
                }
            }
            restore_flags(flags);
            return va;
        }
        q = p;
        p = p->next;
    }

    restore_flags(flags);
    return va;
}

/**
 * 释放page_alloc所分配的页面
 *
 */
int page_free(uint32_t va, int npages)
{
    uint32_t flags;
    uint32_t size = npages * PAGE_SIZE;
    if(npages <= 0)
        return -1;

    struct vmzone *q = NULL, *p = kvmzone;
    if(va < USER_MAX_ADDR)
        p = uvmzone;

    save_flags_cli(flags);
    for(; p != NULL; q = p, p = p->next) {
        if(va >  p->base + p->limit)
            continue;
        if(va == p->base + p->limit) {
            p->limit += size;
            restore_flags(flags);
            return;
        }

        if(va + size == p->base) {
            p->base = va;
            p->limit += size;
            restore_flags(flags);
            return;
        }
        if(va + size < p->base)
            break;
    }
    struct vmzone *x = (struct vmzone *)kmalloc(sizeof(struct vmzone));
    x->base = va;
    x->limit = size;
    x->next = p;

    if(p != NULL) {
        if(q == NULL) {
            if(va < USER_MAX_ADDR)
                uvmzone = x;
            else
                kvmzone = x;
        } else
            q->next = x;
    } else {
        q->next = x;
    }
    restore_flags(flags);
    return 0;
}

/**
 * 检查地址是否合法
 * 合法返回1，否则返回0
 */
int page_check(uint32_t va)
{
    uint32_t flags;
    struct vmzone *p = kvmzone;
    if(va < USER_MAX_ADDR)
        p = uvmzone;

    save_flags_cli(flags);
    for(; p != NULL; p = p->next) {
        if(va >= p->base &&
           va <  p->base + p->limit) {
            restore_flags(flags);
            return 0;
        }
    }
    restore_flags(flags);
    return 1;
}
#endif

/**
 * 把从vaddr开始的虚拟地址，映射到paddr开始的物理地址。
 * 共映射npages页面，把PTE的标志位设为flags
 */
void page_map(uint32_t vaddr, uint32_t paddr, uint32_t npages, uint32_t flags)
{
    for (; npages > 0; npages--){
        *vtopte(vaddr) = paddr | flags;
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
        vaddr += PAGE_SIZE;
    }
}

/**
 * page fault处理函数。
 * 特别注意：此时系统的中断处于打开状态
 */
int do_page_fault(struct context *ctx, uint32_t vaddr, uint32_t code)
{
    uint32_t flags;

#if VERBOSE
    printk("PF:0x%08x(0x%01x)", vaddr, code);
#endif

    if((code & PTE_V) == 0) {
        uint32_t paddr;

        /*检查地址是否合法*/
        if(!page_check(vaddr)) {
            printk("PF: Invalid memory access: 0x%08x(0x%01x)\r\n", vaddr, code);
            return -1;
        }

        /*只要访问用户的地址空间，都代表用户模式访问*/
        if (vaddr < KERN_MIN_ADDR) {
            code |= PTE_U;
        }

        /*搜索空闲帧*/
        paddr = frame_alloc(1);

        if(paddr != SIZE_MAX) {
            /*找到空闲帧*/
            *vtopte(vaddr) = paddr|PTE_V|PTE_W|(code&PTE_U);
            memset(PAGE_TRUNCATE(vaddr), 0, PAGE_SIZE);
            invlpg(vaddr);

#if VERBOSE
            printk("->0x%08x\r\n", *vtopte(vaddr));
#endif

            return 0;
        }
    }

#if VERBOSE
    printk("->????????\r\n");
#else
    printk("PF:0x%08x(0x%01x)->????????\r\n", vaddr, code);
#endif
    return -1;
}

