#ifndef __CUT_H__
#define __CUT_H__
#include "cut-inl.h"
#include <ccan/darray/darray.h>

void cut(struct cnode *root);
int lsearch(rule_set_t *ruleset, struct flow *flow);
int search(struct cnode *root, struct flow *flow);
void traverse(struct cnode *root, void(*traverse_func)(struct cnode *cnode, void *arg, int depth), void *arg, int depth);

struct ctree_info {
    int node_cnt;
    int leaf_cnt;
    int rule_cnt;
    int depth_cnt;
    int max_depth;
    int access_cnt;

    float av_depth;
    float av_access;

    int   cut_type[MAX_CUT_TYPE];
};

void get_tree_info(struct cnode *root);

typedef darray(struct cnode) ctree_t;

ctree_t compact(struct cnode *root);

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


#endif
