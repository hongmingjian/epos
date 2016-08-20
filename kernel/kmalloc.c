/**
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
#include <stddef.h>
#include "cpu.h"
#include "../lib/tlsf/tlsf.h"

static tlsf_t g_kheap;

void *kmalloc(size_t bytes)
{
    uint32_t flags;
    void *ptr;

    save_flags_cli(flags);
    ptr = tlsf_malloc(g_kheap, bytes);
    restore_flags(flags);

    return ptr;
}

void *krealloc(void *oldptr, size_t bytes)
{
    uint32_t flags;
    void *ptr;

    save_flags_cli(flags);
    ptr = tlsf_realloc(g_kheap, oldptr, bytes);
    restore_flags(flags);

    return ptr;
}

void kfree(void *ptr)
{
    uint32_t flags;

    save_flags_cli(flags);
    tlsf_free(g_kheap, ptr);
    restore_flags(flags);
}

void *kmemalign(size_t align, size_t bytes)
{
    uint32_t flags;
	void *ptr;

    save_flags_cli(flags);
    ptr = tlsf_memalign(g_kheap, align, bytes);
    restore_flags(flags);

	return ptr;
}

void init_kmalloc(void *mem, size_t bytes)
{
	g_kheap = tlsf_create_with_pool(mem, bytes);
}
