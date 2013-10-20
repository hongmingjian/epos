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
#include "cpu.h"
#include "global.h"
#include "tlsf/tlsf.h"

static void *g_kern_mem_pool = NULL;

void *kmalloc(size_t bytes)
{
    uint32_t flags;
    void *ptr;

    save_flags_cli(flags);
    ptr = malloc_ex(bytes, g_kern_mem_pool);
    restore_flags(flags);

    return ptr;
}

void *krealloc(void *oldptr, size_t bytes)
{
    uint32_t flags;
    void *ptr;

    save_flags_cli(flags);
    ptr = realloc_ex(oldptr, bytes, g_kern_mem_pool);
    restore_flags(flags);

    return ptr;
}
void kfree(void *ptr)
{
    uint32_t flags;

    save_flags_cli(flags);
    free_ex(ptr, g_kern_mem_pool);
    restore_flags(flags);
}

void init_kmalloc(void *mem, size_t bytes)
{
    g_kern_mem_pool = mem;
    init_memory_pool(bytes, g_kern_mem_pool);
}
