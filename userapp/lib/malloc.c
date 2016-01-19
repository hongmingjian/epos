#include "../lib/tlsf/tlsf.h"

void *malloc(size_t size)
{
    return tlsf_malloc(size);
}
void *calloc(size_t num, size_t size)
{
    return tlsf_calloc(num, size);
}
void *realloc(void *oldptr, size_t size)
{
    return tlsf_realloc(oldptr, size);
}
void free(void *ptr)
{
    tlsf_free(ptr);
}
