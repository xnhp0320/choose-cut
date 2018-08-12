#ifndef _MB_NODE_H_
#define _MB_NODE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mm.h"

//A multi-bit node data structure
#define POINT(X) ((struct mb_node*)(X))
#define NODE_SIZE  (sizeof(struct mb_node))
#define FAST_TREE_FUNCTION
#define STRIDE 6
#define BITMAP_BITS (1 << STRIDE)

struct mb_node{
    uint64_t external;
    uint64_t internal;
    void     *child_ptr;
}__attribute__((aligned(8)));

static inline int count_children(uint64_t bitmap)
{
    return __builtin_popcountll(bitmap);
}

static inline uint32_t count_inl_bitmap(uint32_t bit, int cidr)
{
    uint32_t pos = (1<<cidr) + bit;
    return (pos - 1);
}

static inline uint32_t count_enl_bitmap(uint32_t bits)
{
    return (bits);
}

static inline void set_bit32(uint32_t *bitmap, int pos)
{
    *bitmap |= (1ULL << pos);
}

static inline void clear_bit32(uint32_t *bitmap, int pos)
{
    *bitmap &= ~(1ULL << pos);
}

static inline void set_bit(uint64_t *bitmap, int pos)
{
    *bitmap |= (1ULL << pos);
}

static inline void clear_bit(uint64_t *bitmap, int pos)
{
    *bitmap &= ~(1ULL << pos);
}

static inline uint64_t test_bit(uint64_t bitmap, int pos)
{
    return (bitmap & (1ULL << pos));
}

static inline uint64_t test_bitmap(uint64_t bitmap, int pos)
{
    return (bitmap & (1ULL << pos));
}

#ifdef FAST_TREE_FUNCTION
extern uint64_t fct[1<<(STRIDE - 1)];
extern int pos2cidr[1<<STRIDE];
static inline int tree_function(uint64_t bitmap, uint8_t stride, int *cidr)
{
    uint64_t ret;
    int pos;

    ret = fct[(stride>>1)] & bitmap;
    if(ret){
        pos = __builtin_clzll(ret);
        pos = BITMAP_BITS - 1 - pos;
        *cidr = pos2cidr[pos];
        return pos;
    } else
        return -1;
}

#else
static inline int tree_function(uint64_t bitmap, uint8_t stride, int *cidr)
{
    int i;
    int pos;
    if (bitmap == 0ULL)
        return -1;
    for(i=STRIDE-1;i>=0;i--){
        stride >>= 1;
        pos = count_inl_bitmap(stride, i); 
        if (test_bitmap(bitmap, pos)){
            *cidr = i;
            return pos;
        }
    }
    return -1;
}
#endif
__attribute__((constructor)) void fast_lookup_init(void);
#endif

