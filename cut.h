#ifndef __CUT_H__
#define __CUT_H__
#include "cut-inl.h"

void cut(struct cnode *root);
int lsearch(rule_set_t *ruleset, struct flow *flow);
int search(struct cnode *root, struct flow *flow);



#endif
