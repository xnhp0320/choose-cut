#ifndef __CUT_H__
#define __CUT_H__
#include "cut-inl.h"

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
};

void get_tree_info(struct cnode *root);

#endif
