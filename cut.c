#include "cut.h"
#include "utils-inl.h"
#include "mem.h"
#include "cut-split.h"
#include "cut-bitmap.h"


void cut_node(struct cnode *n, struct cut_aux *aux)
{


    if(n->ruleset.num <= BUCKETSIZE) {
        n->leaf = 1;
        return;
    }
    
    int i;
    int t;
    double min_t, min[MAX_CUT_TYPE], me[MAX_CUT_TYPE];
    int chose_dim[MAX_CUT_TYPE];
    int chose_t;
   
    for(t = 0; t < MAX_CUT_TYPE; t ++ ) {
        for(i = 0; i < DIM; i ++) {
            me[t] = cuts[t].match_expect(n, i, aux);
            if(i == 0) {
                min[t] = me[t];
                chose_dim[t] = i;
            } else {
                if(me[t] < min[t]) {
                    min[t] = me[t];
                    chose_dim[t] = i;
                }
            }
        }
        //LOG("for cut %d, min %.2f, chose_dim %d\n", t, min[t], chose_dim[t]);
    }

    chose_t = 0;
    min_t = (double)n->ruleset.num;

    for(t = 0; t < MAX_CUT_TYPE; t ++) {
        if(min[t] < min_t) {
            min_t = min[t];
            chose_t = t;
        }
    }

    if(memory_constraints) {
        int mem_quant[MAX_CUT_TYPE];
        for(t = 0; t < MAX_CUT_TYPE; t++) {
            if(cuts[t].all_fits_bs(aux, chose_dim[t])) {
                mem_quant[t] = cuts[t].mem_quant(aux, chose_dim[t]); 
            } else {
                mem_quant[t] = CHILDCOUNT;
            }
        }

        for(t = 0; t < MAX_CUT_TYPE; t++ ) {
            if(mem_quant[t] < mem_quant[0]) {
                mem_quant[0] = mem_quant[t];
                chose_t = t;
            }
        }
    }


    //LOG("Choose %d Cut method %d\n", chose_dim[chose_t], chose_t);    
    n->dim = chose_dim[chose_t];
    n->type = chose_t;
    cuts[chose_t].cut_node(n, chose_dim[chose_t], aux);
}

void cut(struct cnode *root)
{
    struct cut_aux aux;
    int t;
    for(t = 0; t < MAX_CUT_TYPE; t ++ )
        cuts[t].aux_init(root, &aux);

    cut_node(root, &aux);
    
}

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


int search(struct cnode *root, struct flow *flow)
{
    struct cnode *n = root;
    while(n && !n->leaf) {
        switch (n->type) {
            case BITMAP_CUT:
                n = even_search(n, flow);
                break;
            case SPLIT_CUT:
                n = split_search(n, flow);
                break;
        }
    }

    if(!n)
        return -1;
    return lsearch(&n->ruleset, flow);
}

void traverse(struct cnode *root, void(*traverse_func)(struct cnode *cnode, void *arg, int depth), void *arg, int depth)
{
    traverse_func(root, arg, depth);
    cuts[root->type].traverse(root, traverse_func, arg, depth);
}

static
void get_tree_info_traverse(struct cnode *n, void *arg, int depth)
{
    struct ctree_info *info = (struct ctree_info*)arg;

    if(depth > info->max_depth)
        info->max_depth = depth;

    info->node_cnt++;
    if(n->leaf) {
        info->leaf_cnt ++;
        info->rule_cnt += n->ruleset.num;
        info->depth_cnt += depth;
        info->access_cnt += (depth + n->ruleset.num);
    }
}

void get_tree_info(struct cnode *root)
{
    struct ctree_info info = {0};
    traverse(root, get_tree_info_traverse, &info, 0);

    LOG("NODE %d\n", info.node_cnt);
    LOG("LEAF %d\n", info.leaf_cnt);
    LOG("RULES %d\n", info.rule_cnt);
    LOG("max Depth %d\n", info.max_depth);

    info.av_depth = (float)info.depth_cnt / info.leaf_cnt; 
    info.av_access = (float)info.access_cnt / info.leaf_cnt; 

    LOG("av depth %.2f\n", info.av_depth);
    LOG("av access %.2f\n", info.av_access);

}

struct ct_info {
    ctree_t ct;
    int copied;
};

static
void compact_traverse(struct cnode *n, void *arg, int depth)
{
    struct ct_info *ci = (struct ct_info*)arg;
    struct cnode *copied_cn;

    darray_append(ci->ct, *n);
    ci->copied ++;

    if(!n->leaf) {
        int childs = cuts[n->type].count_childs(n);
        struct cnode *dest = darray_make_room(ci->ct, childs);
        copied_cn = (struct cnode*)&darray_item(ci->ct, ci->copied);

        cuts[n->type].set_child_ptr(n, copied_cn, dest);
        ci->ct.size += childs;
    }
}

ctree_t compact(struct cnode *root)
{
    struct ct_info ci = {.copied = 0};
    darray_init(ci.ct);

    traverse(root, compact_traverse, &ci, 0);
    LOG("%d nodes copied\n", ci.copied);
    return ci.ct;
}


