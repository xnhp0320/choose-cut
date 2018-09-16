#ifndef _CUT_HI_H_
#define _CUT_HI_H_

#include "cut-inl.h"

static inline struct cnode *hi_next(struct cnode *n, int idx)
{
    uint64_t mask = (1ULL << idx) -1;
    int offset = __builtin_popcount(n->hn.bitmap & mask);
    return (struct cnode *)(n->hn.child_ptr) + offset;
}

static inline struct cnode *hi_search(struct cnode *n, struct flow *f)
{
    if(f->key[n->dim] < n->hn.start)
        return NULL;

    int idx = (f->key[n->dim] - n->hn.start)  >> n->hn.shift ;

    return hi_next(n, idx);

}


#endif
