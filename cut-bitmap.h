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

#endif
