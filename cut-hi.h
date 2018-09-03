#ifndef _CUT_HI_H_
#define _CUT_HI_H_

#include "cut-inl.h"

static inline struct cnode *hi_next(struct cnode *n, int idx)
{
    uint8_t mask = (1<<idx) -1;
    int offset = __builtin_popcount(n->hn.bitmap & mask);
    return (struct cnode *)(n->hn.child_ptr) + offset;
}




#endif
