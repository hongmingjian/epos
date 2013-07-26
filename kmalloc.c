#include "cpu.h"
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
