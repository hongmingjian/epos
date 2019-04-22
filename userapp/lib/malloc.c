#include <string.h>
#include "../lib/tlsf/tlsf.h"

tlsf_t g_heap;

void *malloc(size_t bytes)
{
    return tlsf_malloc(g_heap, bytes);
}
void *calloc(size_t num, size_t bytes)
{
    void *p = malloc(num*bytes);
    if(p != NULL)
        memset(p, 0, num*bytes);
    return p;
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
