#include "cut-split.h"
#include "utils-inl.h"
#include "mem.h"




static bool split_aux_heap_less(const void *a, const void *b)
{
    struct range1d *r1 = (struct range1d *)a;
    struct range1d *r2 = (struct range1d *)b;

    return r1->high < r2->high;
}

static int _range_compare(const void *a, const void *b)
{
    const struct range1d *r1 = (const struct range1d*)a;
    const struct range1d *r2 = (const struct range1d*)b;

    if(r1->low != r2->low)
        if(r1->low < r2->low)
            return -1;
        else 
            return 1;
    else if (r1->high != r2->high) {
        if(r1->high < r2->high)
            return -1;
        else
            return 1;
    }
    return 0;
}

static int unique_ranges(struct range1d *ranges, int num)
{
    int i; 
    int unique_num = 1;
    for(i = 1; i < num; i ++) {
        if(ranges[i].low != ranges[unique_num -1].low \
                || ranges[i].high != ranges[unique_num -1].high) {
            ranges[unique_num].low = ranges[i].low;
            ranges[unique_num].high = ranges[i].high;
            unique_num ++;
        } else {
            ranges[unique_num -1].weight ++;
        }
    }
    return unique_num;
}



static int 
split_aux_init(struct cnode *n, struct cut_aux* aux)
{
    aux->split_aux.h = heap_init(split_aux_heap_less);
    if(!aux->split_aux.h) {
        return -1;
    }

    darray_init(aux->split_aux.ranges);
    darray_growalloc(aux->split_aux.ranges, n->ruleset.num);

    int ret;
    ret = rule_set_init(&aux->split_aux.right_rules, n->ruleset.num);
    if(ret == -1) {
        PANIC("memory alloc fail\n");
    }

    ret = rule_set_init(&aux->split_aux.left_rules, n->ruleset.num);
    if(ret == -1) {
        rule_set_free(&aux->split_aux.right_rules);
        PANIC("memory alloc fail\n");
    }

    return 0;
}



static void
find_min_me_do(double *min_me, unsigned int *s, struct range1d range[], 
                    double curr, unsigned int scurr, unsigned int left, \
                    unsigned int right)
{
    if(*min_me > curr) {
        *min_me = curr;
        *s = scurr;
        range[0].weight = left;
        range[1].weight = right;
    }
}

static void
split_disable_snode(struct snode *n)
{
    n->flags = 0;
}

static void
split_enable_snode(struct snode *n)
{
    n->flags = 1;
}

static bool
split_snode_enabled(struct snode *n)
{
    return n->flags == 1;
}

static double 
split_match_expect_1d(struct snode *n, int dim, struct cut_aux *aux)
{
    int i;
    d_ranges_t ranges;
    ranges = aux->split_aux.ranges;
    for(i = 0; i < n->ruleset.num; i++) {
        darray_item(ranges, i).low = n->ruleset.ruleList[i].range[dim][0];
        darray_item(ranges, i).high = n->ruleset.ruleList[i].range[dim][1];
        darray_item(ranges, i).weight = 1;
    }
    qsort(ranges.item, n->ruleset.num, sizeof(struct range1d), _range_compare);
    int uniq_num = unique_ranges(ranges.item, n->ruleset.num);

    if(uniq_num == 1) {
        return (double)n->ruleset.num;
    }
    
    double min_me = (double)n->ruleset.num, curr_me; 
    unsigned int left = 0, dup=0, right=0, split, e;
    unsigned int s = darray_item(ranges, 0).low;
    struct range1d *top, *r, *o;

    struct heap *h = aux->split_aux.h;
    heap_push(h, &darray_item(ranges, 0));

    n->r[0].low = darray_item(ranges, 0).low;
    n->r[1].high = darray_item(ranges, 0).high;
    /* this just gives split an initial value */
    split = n->r[0].low;

    i = 1;
    int j;
    for(; i < uniq_num; i ++) {
        r = &darray_item(ranges, i);
        while(h->len != 0 && 
                (top = (struct range1d*)heap_peek(h))->high < r->low) {
            e = top->high;
            n->r[1].high = e > n->r[1].high ? e : n->r[1].high;

            while(h->len != 0 && \
                (top = (struct range1d*)heap_peek(h))->high == e) {
                left += top->weight;
                heap_pop(h);
            }

            dup = 0;
            for(j=0; j < h->len; j ++) {
                o = (struct range1d*)h->data[j];
                dup += o->weight;
            }

            right = n->ruleset.num  - left;

            curr_me = (right + left + dup) / 2.;
            find_min_me_do(&min_me, &split, n->r, \
                            curr_me, e, left+dup, right);
        }

        if(r->low == s) {
            heap_push(h, r);
            continue;
        }

        dup = 0;
        for(j=0; j < h->len; j ++) {
            o = (struct range1d*)h->data[j];
            dup += o->weight;
        }

        right = n->ruleset.num - left;

        curr_me = (right + left + dup)/2.;
        find_min_me_do(&min_me, &split, n->r, \
                        curr_me, r->low -1, left+dup, right);
        s = r->low;
        heap_push(h, r);
    }

    while(h->len > 1) {
        top = (struct range1d*)heap_peek(h);
        e = top->high;
        n->r[1].high = e > n->r[1].high ? e : n->r[1].high;

        while(h->len != 0 && \
                (top = (struct range1d*)heap_peek(h))->high == e) {
            left += top->weight;
            heap_pop(h);
        }

        if(h->len) {
            dup = 0;
            for(j = 0; j < h->len; j ++) {
                o = (struct range1d*)h->data[j];
                dup += o->weight;
            }

            right = n->ruleset.num - left;
            curr_me = (right+left + dup)/2.;
            find_min_me_do(&min_me, &split, n->r, \
                    curr_me, e, left+dup, right);
        }
    }

    if(h->len) {
        top = (struct range1d*)heap_peek(h);
        n->r[1].high = top->high > n->r[1].high ? top->high : n->r[1].high;
        heap_pop(h);
    }

    assert(h->len == 0);
    
    n->r[0].high = split;
    n->r[1].low = split + 1;

    return min_me;
}

static inline void
_split_cut_1d(rule_set_t *ors, int dim, 
                struct range1d *left, struct range1d *right,
                rule_set_t *lrs, rule_set_t *rrs)
{
    int i;
    rule_t rule;
    for(i = 0; i < ors->num; i ++) {
        rule = ors->ruleList[i];
        if(!(rule.range[dim][1] < left->low || 
                rule.range[dim][0] > left->high)) {

            if(rule.range[dim][1] > left->high)
                rule.range[dim][1] = left->high;

            if(rule.range[dim][0] < left->low)
                rule.range[dim][0] = left->low;

            append_rules(lrs, &rule);  
        }

        rule = ors->ruleList[i];
        if(!(rule.range[dim][1] < right->low || 
                rule.range[dim][0] > right->high)) {

            if(rule.range[dim][1] > right->high)
                rule.range[dim][1] = right->high;

            if(rule.range[dim][0] < right->low)
                rule.range[dim][0] = right->low;

            append_rules(rrs, &rule);  
        }
    }
}

static void 
split_cut_1d(struct snode *n, int dim, struct snode *left, struct snode *right)
{

    _split_cut_1d(&n->ruleset, dim, &n->r[0], &n->r[1], &left->ruleset, &right->ruleset);  
    
    assert(left->ruleset.num == n->r[0].weight);
    assert(right->ruleset.num == n->r[1].weight);
}

static void
split_match_build_childs(struct snode *n, struct cut_aux *cut_aux)
{
    double me, mec; //mec: minimal expect value for childs
    struct range1d min_r[2];
    int min_dim = -1, i;

    /* 
    if(n->ruleset.num < BUCKETSIZE)
        split_disable_snode(n);
    else
        split_enable_snode(n);
    */
    split_enable_snode(n);

    for(i = 0; i < DIM; i ++ ) {
        me = split_match_expect_1d(n, i, cut_aux); 
        if(min_dim == -1) {
            min_r[0] = n->r[0];
            min_r[1] = n->r[1];
            min_dim = i;
            mec = me;
        } else if( me < mec) {
            min_r[0] = n->r[0];
            min_r[1] = n->r[1];
            min_dim = i;
            mec = me;
        }
    }

    if(double_equals(mec, (double)n->ruleset.num)) {
        split_disable_snode(n);
    }

    n->dim = min_dim;
    n->r[0] = min_r[0];
    n->r[1] = min_r[1];
}

static double
split_match_expect_subtree(struct stree *t)
{
    int rules = 0;
    int childs = 2;

    if(split_snode_enabled(&t->left)) {
        rules += t->left.r[0].weight;
        rules += t->left.r[1].weight;
        childs++;
    } else {
        rules += t->root.r[0].weight;
    }

    if(split_snode_enabled(&t->right)) {
        rules += t->right.r[0].weight;
        rules += t->right.r[1].weight;
        childs++;
    } else {
        rules += t->root.r[1].weight;
    }

    return (double)rules/childs;
}

static double
split_match_expect(struct cnode *node, int dim, struct cut_aux *cut_aux)
{
    struct split_aux *aux = &cut_aux->split_aux;

    struct snode *n = &aux->strees[dim].root;
    double me;

    if(node == 0x637350)
        LOG("HELLO");

    n->ruleset = node->ruleset;
    n->dim = dim;
    
    aux->left_rules.num = 0;
    aux->right_rules.num = 0;

    aux->strees[dim].left.ruleset = aux->left_rules;
    aux->strees[dim].right.ruleset = aux->right_rules;

    split_enable_snode(n);

    me = split_match_expect_1d(n, dim, cut_aux); 
    if(double_equals(me, (double)n->ruleset.num)) {
        split_disable_snode(n);
        split_disable_snode(&aux->strees[dim].left);
        split_disable_snode(&aux->strees[dim].right);
        return me;
    }

    if(memory_constraints && n->r[0].weight <= BUCKETSIZE && \
            n->r[1].weight <= BUCKETSIZE) {
        split_disable_snode(&aux->strees[dim].left);
        split_disable_snode(&aux->strees[dim].right);
        return me;
    }

    split_cut_1d(n, dim, &aux->strees[dim].left, &aux->strees[dim].right);

    split_match_build_childs(&aux->strees[dim].left, cut_aux);
    split_match_build_childs(&aux->strees[dim].right, cut_aux);
    return split_match_expect_subtree(&aux->strees[dim]);
}

static void
split_cut_recursive(struct cnode *cn, int childs, struct cut_aux *aux)
{
    int i;
    for(i = 0; i < childs; i++) {
        remove_redund(&cn[i].ruleset);
        cut_node(&cn[i], aux);
    }
}

static int
split_cut(struct cnode *n, int dim, struct cut_aux *cut_aux)
{
    if(n == 0x635e80)
        LOG("HELLO");

    struct split_aux *aux = &cut_aux->split_aux;
    struct stree *tree = &aux->strees[dim];
    struct sp_node *sp = &n->sp;

    int childs = 2;
    if(split_snode_enabled(&tree->left)) {
        childs++;
    }
    if(split_snode_enabled(&tree->right)) {
        childs++;
    }

    sp->child_ptr = bc_calloc(childs, sizeof(*n));
    if(!sp->child_ptr) {
        PANIC("Memory alloc fail\n");
    }

    tree->root.ruleset = n->ruleset;

    tree->left.ruleset = aux->left_rules;
    tree->right.ruleset = aux->right_rules;
    tree->left.ruleset.num = 0;
    tree->right.ruleset.num = 0;

    split_cut_1d(&tree->root, dim, &tree->left, &tree->right);

    struct cnode *cn;
    cn = (struct cnode *)n->sp.child_ptr;
    int i = 0;

    if(split_snode_enabled(&tree->left)) {
        rule_set_init(&cn[0].ruleset, tree->left.r[0].weight);
        rule_set_init(&cn[1].ruleset, tree->left.r[1].weight);

        _split_cut_1d(&tree->left.ruleset, tree->left.dim, &tree->left.r[0], &tree->left.r[1], \
                        &cn[0].ruleset, &cn[1].ruleset);
        i+= 2;
    } else {
        rule_set_copy(&cn[i].ruleset, &tree->left.ruleset);
        i+= 1;
    }

    if(split_snode_enabled(&tree->right)) {
        rule_set_init(&cn[i].ruleset, tree->right.r[0].weight);
        rule_set_init(&cn[i+1].ruleset, tree->right.r[1].weight);

        _split_cut_1d(&tree->right.ruleset, tree->right.dim, &tree->right.r[0], &tree->right.r[1], \
                        &cn[i].ruleset, &cn[i+1].ruleset);
    } else {
        rule_set_copy(&cn[i].ruleset, &tree->right.ruleset);
    }
    
    split_cut_recursive(cn, childs, cut_aux);

    return 0;
}


static int
split_mem_quant(struct cut_aux *aux, int dim)
{
    struct stree *tree = &aux->split_aux.strees[dim];
    int ret = 2;
    if(split_snode_enabled(&tree->left)) {
        ret ++;
    }

    if(split_snode_enabled(&tree->right)) {
        ret ++;
    }

    return ret;
}

static bool
split_fits_bs(struct cut_aux *aux, int dim)
{
    struct stree *t = &aux->split_aux.strees[dim];

    if(split_snode_enabled(&t->left)) {
        if(t->left.r[0].weight > BUCKETSIZE || \
            t->left.r[1].weight > BUCKETSIZE) {
            return false;
        }
    } else {
        if(t->root.r[0].weight > BUCKETSIZE)
            return false;
    }

    if(split_snode_enabled(&t->right)) {
        if(t->right.r[0].weight > BUCKETSIZE || \
            t->right.r[1].weight > BUCKETSIZE) {
            return false;
        }
    } else {
        if(t->root.r[1].weight > BUCKETSIZE)
            return false;
    }

    return true;
}

__attribute__((constructor)) static void register_cut_method(void)
{
    cuts[SPLIT_CUT].aux_init = split_aux_init;
    cuts[SPLIT_CUT].cut_node = split_cut;
    cuts[SPLIT_CUT].match_expect = split_match_expect;
    cuts[SPLIT_CUT].mem_quant = split_mem_quant;
    cuts[SPLIT_CUT].all_fits_bs = split_fits_bs;
}


