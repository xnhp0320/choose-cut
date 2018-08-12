#ifndef _CUT_BITMAP_H_
#define _CUT_BITMAP_H_

#include "cut-inl.h"

static inline struct cnode *next_inl(struct cnode *n, int idx){
    uint64_t mask = (1ULL << idx ) -1;
    int offset = __builtin_popcountll(n->mb.internal & mask);
    return (struct cnode *)(n->mb.child_ptr) - (offset) - 1;
}

static inline struct cnode *next_exl(struct cnode *n, int idx) {
    uint64_t mask = (1ULL << (idx)) -1;
    int offset = __builtin_popcountll(n->mb.external & mask);
    return (struct cnode *)(n->mb.child_ptr) + offset;
}

int even_cut(struct cnode *n, int dim, struct cut_aux *aux);

extern int dim_bits[DIM];

static inline uint32_t even_search_lshift(uint32_t dest, int bits, int lshift)
{
    switch(bits) {
        case 8:
            dest = (uint8_t)(dest << lshift);
            break;
        case 16:
            dest = (uint16_t)(dest << lshift);
            break;
        default:
            dest <<= lshift;
            break;
    }
    return dest;
}

/* search to the next node */
static inline struct cnode *even_search(struct cnode *n, struct flow *f)
{
    uint8_t stride = f->key[n->dim] >> (dim_bits[n->dim] - STRIDE);
    if(test_bitmap(n->mb.external, count_enl_bitmap(stride))) {
        f->key[n->dim] = even_search_lshift(f->key[n->dim], dim_bits[n->dim], STRIDE);
        return next_exl(n, count_enl_bitmap(stride));
    }

    int cidr = 0;
    int pos = tree_function(n->mb.internal, stride, &cidr);
    if(pos != -1) {
        f->key[n->dim] = even_search_lshift(f->key[n->dim], dim_bits[n->dim], cidr);
        return next_inl(n, pos);
    }
    return NULL;
}

#endif
