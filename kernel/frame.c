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
#include "kernel.h"
#include "bitmap.h"

static struct pmzone {
 uint32_t base;
 uint32_t limit;
 struct bitmap *bitmap;
} pmzone[RAM_ZONE_LEN/2];

uint32_t init_frame(uint32_t brk)
{
    int i, z = 0;

    for(i = 0; i < RAM_ZONE_LEN; i += 2) {
        if(g_ram_zone[i+1] - g_ram_zone[i] == 0)
            break;

        pmzone[z].base = g_ram_zone[i];
        pmzone[z].limit = g_ram_zone[i+1]-g_ram_zone[i];
        uint32_t bit_cnt = pmzone[z].limit/PAGE_SIZE;
        uint32_t size = PAGE_ROUNDUP(bitmap_buf_size(bit_cnt));
		bit_cnt -= size/PAGE_SIZE;
        uint32_t paddr = pmzone[z].base;
        pmzone[z].base += size;
        pmzone[z].limit -= size;
        if(pmzone[z].limit == 0)
            continue;

        printk("RAM: 0x%08x - 0x%08x (%d frames)\r\n",
                pmzone[z].base, pmzone[z].base+pmzone[z].limit,
                pmzone[z].limit/PAGE_SIZE);

        page_map(brk, paddr, size/PAGE_SIZE, L2E_V|L2E_W|L2E_C);
        pmzone[z].bitmap=bitmap_create_in_buf(bit_cnt, (void *)brk, 0);
        brk += size;
        z++;
    }

    return brk;
}

/**
 * 在指定的物理地址pa分配nframes个连续帧
 * 失败返回SIZE_MAX，成功返回pa
 */
uint32_t frame_alloc_in_addr(uint32_t pa, uint32_t nframes)
{
    int z;
    uint32_t flags;

    save_flags_cli(flags);
    for(z = 0; z < RAM_ZONE_LEN/2; z++) {
        if(pa >= pmzone[z].base &&
           pa <  pmzone[z].base + pmzone[z].limit) {
            uint32_t idx = (pa - pmzone[z].base) / PAGE_SIZE;
            if(bitmap_none(pmzone[z].bitmap, idx, nframes)) {
                bitmap_set_multiple(pmzone[z].bitmap, idx, nframes, 1);
                restore_flags(flags);
                return pa;
            }
        }
    }
    restore_flags(flags);

    return SIZE_MAX;
}

/**
 * 分配nframes个连续的帧
 * 失败返回SIZE_MAX，成功返回帧的起始地址
 */
uint32_t frame_alloc(uint32_t nframes)
{
    int z;
    uint32_t flags;

    save_flags_cli(flags);
    for(z = 0; z < RAM_ZONE_LEN/2; z++) {
        if(pmzone[z].limit == 0)
            break;
        uint32_t idx = bitmap_scan(pmzone[z].bitmap, 0, nframes, 0);
        if(idx != SIZE_MAX) {
            bitmap_set_multiple(pmzone[z].bitmap, idx, nframes, 1);
            restore_flags(flags);
            return pmzone[z].base + idx * PAGE_SIZE;
        }
    }
    restore_flags(flags);

    return SIZE_MAX;
}

/**
 * 释放frame_alloc所分配的帧
 */
void frame_free(uint32_t paddr, uint32_t nframes)
{
    uint32_t z;
    uint32_t flags;

    save_flags_cli(flags);
    for(z = 0; z < RAM_ZONE_LEN/2; z++) {
        if(pmzone[z].limit == 0)
            break;
        if(paddr >= pmzone[z].base &&
           paddr <  pmzone[z].base+pmzone[z].limit) {
            uint32_t idx = (paddr - pmzone[z].base) / PAGE_SIZE;
            /* XXX - 确认之前是否空闲
            if(bitmap_any(pmzone[z].bitmap, idx, nframes)) {
                restore_flags(flags);
                return;
            }*/
            bitmap_set_multiple(pmzone[z].bitmap, idx, nframes, 0);
            restore_flags(flags);
            return;
        }
    }
    restore_flags(flags);
}

void swapper(void *pv)
{
    uint32_t flags, va = PAGE_TRUNCATE(rand(USER_MIN_ADDR,
                                            USER_MAX_ADDR-PAGE_SIZE));
    struct vmzone *z;
	while(1) {
		sys_sem_wait(sem_swapper);

		do {
			va += PAGE_SIZE;
			if(va >= USER_MAX_ADDR)
				va = USER_MIN_ADDR;
			z = page_zone(va);
		} while(z == NULL);

		do {
			save_flags_cli(flags);

			if((z->fp != NULL) &&
			   (z->prot != PROT_NONE) &&
			   ((z->flags & MAP_SHARED) ||
				((z->flags & MAP_PRIVATE) &&
				 (z->prot & PROT_WRITE) == 0))) {
				uint32_t end =  z->base+z->limit;

				restore_flags(flags);
				int found = 0;
				for(; va < end; va+=PAGE_SIZE)
					if((PTD[va>>PGDR_SHIFT] & L1E_V) &&
					   ((*vtopte(va)) & L2E_V)) {
						found = 1;
						break;
					}

				if(found)
					break;

				save_flags_cli(flags);
			}

			z = z->next;
			if(z == NULL)
				z = uvmzone;
			va = z->base;
			restore_flags(flags);
		} while(1);

		if(z->flags & MAP_SHARED) {
			if(z->fp->fs->seek(z->fp, z->offset+va-z->base, SEEK_SET) >= 0 &&
			   z->fp->fs->write(z->fp, (void *)va, PAGE_SIZE))
				;
			else
				;
		}

		uint32_t pa=PAGE_TRUNCATE(*vtopte(va));

		page_unmap(va, 1);
		frame_free(pa, 1);

		sys_sem_signal(sem_ram);
	}

	sys_task_exit(0);
}
