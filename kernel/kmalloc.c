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

void *kmalloc(size_t bytes)
{
    uint32_t flags;
    void *ptr;

    save_flags_cli(flags);
    ptr = tlsf_malloc(bytes);
    restore_flags(flags);

    return ptr;
}

void *krealloc(void *oldptr, size_t bytes)
{
    uint32_t flags;
    void *ptr;

    save_flags_cli(flags);
    ptr = tlsf_realloc(oldptr, bytes);
    restore_flags(flags);

    return ptr;
}

void kfree(void *ptr)
{
    uint32_t flags;

    save_flags_cli(flags);
    tlsf_free(ptr);
    restore_flags(flags);
}

void *aligned_kmalloc(size_t bytes, size_t align)
{
    void *mem = kmalloc(bytes+align-1+sizeof(void*));
	if(mem == NULL)
		return mem;
    uint32_t ptr = ((size_t)((char*)mem+sizeof(void*)+align-1)) & ~ (align-1);
	((void**)ptr)[-1] = mem;
    return (void *)ptr;
}

void aligned_kfree(void *ptr)
{
    kfree(((void**)ptr)[-1]);
}

void init_kmalloc(void *mem, size_t bytes)
{
    init_memory_pool(bytes, mem);
}
