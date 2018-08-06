#include <stdlib.h>
#include "mem.h"

__attribute__((weak)) void *bc_calloc(size_t elem, size_t cnt)
{
    return calloc(elem, cnt);
}

__attribute__((weak)) void *bc_realloc(void *orig, size_t cnt)
{
    return realloc(orig, cnt);
}

__attribute__((weak)) void bc_free(void *ptr)
{
    free(ptr);
}

__attribute__((weak)) void *bc_malloc(size_t size)
{
    return malloc(size);
}
