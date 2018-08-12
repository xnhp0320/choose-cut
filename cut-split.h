#ifndef _CUT_SPLIT_H_
#define _CUT_SPLIT_H_

#include "cut-inl.h"


static inline struct cnode *split_next(struct cnode *n, int idx)
{
    uint8_t mask = (1<<idx) -1;
    int offset = __builtin_popcount(n->sp.shape & mask);
    return (struct cnode *)(n->sp.child_ptr) + offset;
}

static inline struct cnode *split_search(struct cnode *n, struct flow *f)
{
    int idx = 0;
    if(f->key[n->dim] > n->sp.root_split) {
        idx ++;
    }

    idx <<=1;
    if(idx && n->sp.dim_right != DIM) {
        if(f->key[n->sp.dim_right] > n->sp.right_split) {
            idx++;
        }
    }
    
    if(!idx && n->sp.dim_left != DIM) {
        if(f->key[n->sp.dim_left] > n->sp.left_split) {
            idx++;
        }
    }

    return split_next(n, idx);
}

#endif
