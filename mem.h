#ifndef __MEM_H__
#define __MEM_H__

/* memory management */
/* these are weak symbols, can be override by 
 * user 
 */
#include <stdlib.h>
#include <jemalloc/jemalloc.h>

void *bc_calloc(size_t, size_t);
void *bc_realloc(void *, size_t);
void  bc_free(void*);
void *bc_malloc(size_t size);

#endif
