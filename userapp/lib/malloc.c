#include "../lib/tlsf/tlsf.h"

tlsf_t g_heap;

void *malloc(size_t bytes)
{
    return tlsf_malloc(g_heap, bytes);
}
void *calloc(size_t num, size_t bytes)
{
    return malloc(num*bytes);
}
void* memalign(size_t align, size_t bytes)
{
	return tlsf_memalign(g_heap, align, bytes);
}
void *realloc(void *oldptr, size_t bytes)
{
    return tlsf_realloc(g_heap, oldptr, bytes);
}
void free(void *ptr)
{
    tlsf_free(g_heap, ptr);
}
