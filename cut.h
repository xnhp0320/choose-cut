#ifndef __CUT_H__
#define __CUT_H__
#include "cut-inl.h"
#include <ccan/darray/darray.h>

void cut(struct cnode *root);
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

static inline
int lsearch(rule_set_t *ruleset, struct flow *flow)
{
    int i;
    for(i=0;i<ruleset->num;i++) {
        if(flow->key[0] >= ruleset->ruleList[i].range[0][0] 
                && flow->key[0] <= ruleset->ruleList[i].range[0][1]
                && flow->key[1] >= ruleset->ruleList[i].range[1][0]
                && flow->key[1] <= ruleset->ruleList[i].range[1][1]
                && flow->key[2] >= ruleset->ruleList[i].range[2][0]
                && flow->key[2] <= ruleset->ruleList[i].range[2][1]
#if DIM == 5 
                && flow->key[3] >= ruleset->ruleList[i].range[3][0]
                && flow->key[3] <= ruleset->ruleList[i].range[3][1]
                && flow->key[4] >= ruleset->ruleList[i].range[4][0]
                && flow->key[4] <= ruleset->ruleList[i].range[4][1]
#endif
          ){
            return ruleset->ruleList[i].pri;
        }
    }

    return -1;
}


#endif
