#include "cut-bitmap.h"
#include "utils-inl.h"
#include "mem.h"

int dim_bits[DIM] = { 32, 32, 16, 16, 8};

static void get_rule_hist(rule_set_t *ruleset, int dim, struct rule_hist *hist)
{
    int i, j;
    range_t *r;
    prefix_iter_t iter;
    prefix_t p;
    int idx;
    int flag;

    memset(hist, 0, sizeof(*hist));
    for(i = 0; i < ruleset->num; i ++) {
        r = ruleset->ruleList[i].range[dim];
        iter = range_to_prefix_iter(r);
        do {
            p = get_prefix(&iter, dim_bits[dim]);
            if(p.len >= STRIDE)  {
                idx = (p.prefix >> (dim_bits[dim] - STRIDE)) + INL_OFFSET;
            } else {
                idx = count_inl_bitmap(p.prefix >> (dim_bits[dim] - p.len), p.len); 
            }
            hist->child_rulecount[idx] ++;
            iter = get_next_prefix_iter(&iter, &flag); 
        }while(flag);
    }

    for(i = 0; i < CHILDCOUNT; i++) {
        if(hist->child_rulecount[i]) {
            j = i;
            while(j) {
                j = (j - 1)/2;
                if(hist->child_rulecount[j]) {
                    hist->child_rulecount[i] += hist->child_rulecount[j];
                    break;
                }
            }

            hist->childs++;
            hist->rules += hist->child_rulecount[i];
        }
    }
}

static double get_match_expect(struct rule_hist *hist)
{
    //return (double)hist->rules/hist->childs;

    int i;
    double ret = 0;
    for(i = 0; i < CHILDCOUNT; i ++) {
        if(hist->child_rulecount[i]) {
            ret += (double)(hist->child_rulecount[i] * \
                    hist->child_rulecount[i])/hist->rules;
        }
    }
    return ret;
}

static double even_match_expect(struct cnode *n, int dim, struct cut_aux *aux)
{
    //if(n == 0x858300)
    //    LOG("HELLO");
    get_rule_hist(&n->ruleset, dim, &aux->hist[dim]);
    return get_match_expect(&aux->hist[dim]);
}


static prefix_t inl_idx_to_prefix[INL_OFFSET];
__attribute__((constructor)) static void init_tbl_for_inl_idx_to_prefix(void)
{
    int i, cidr = 0;
    int idx = 0;
    for(cidr = 0; cidr <= STRIDE -1; cidr ++) {
        for(i = 0; i < (1<<cidr); i ++) {
            inl_idx_to_prefix[idx].prefix = i;
            inl_idx_to_prefix[idx].len = cidr;
            idx ++;
        }
    }
}

static prefix_t idx_to_prefix(int idx, int dim)
{
    prefix_t p;
    if(idx < INL_OFFSET) {
        p = inl_idx_to_prefix[idx];
        p.prefix <<= (dim_bits[dim] - p.len);
    } else {
        p.prefix = (idx - INL_OFFSET) << (dim_bits[dim] - STRIDE);
        p.len = STRIDE;
    }
    return p;
}

static void push_inl_rules_even(int idx, int dim, \
                                struct cnode *curr, rule_t *r, struct cut_aux *aux)
{
    struct cnode *n;
    int i = idx * 2 + 1;
    int j = 2;
    int k = 0;
    int start;

    /* the first node is the node with *idx*, 
     * the second line is a row with 2 nodes, starting with idx of idx *2 +1, 
     * the third line is a row with 2^2 nodes, starting with the last start *2 +1,
     * and ....
     */

    while(i < CHILDCOUNT) {
        start = i;
        for(k = 0; k < j; k ++ ) {
            if(aux->hist[dim].child_rulecount[i]) {
                if(i < INL_OFFSET) {
                    n = next_inl(curr, i);
                    append_rules(&n->ruleset, r);
                } else {
                    n = next_exl(curr, i - INL_OFFSET);
                    append_rules(&n->ruleset, r);
                }
            }
            i ++;
        }
        j *= 2;
        i = start * 2 +1;
    }
}


static void push_rules_even(struct cnode *curr, int dim, struct cut_aux *aux)
{
    int i;
    struct cnode *n;
    int num;

    for(i = 0; i < CHILDCOUNT; i++) {
        if(aux->hist[dim].child_rulecount[i]) {
            if(i < INL_OFFSET) {
                n = next_inl(curr, i);
            } else {
                n = next_exl(curr, i - INL_OFFSET);
            }

            num = aux->hist[dim].child_rulecount[i];
            n->ruleset.ruleList = bc_calloc(num, sizeof(rule_t));
            if(!n->ruleset.ruleList) {
                PANIC("memory alloc fail\n"); 
            }
            n->ruleset.cap = num;
        }
    }

    range_t *r;
    prefix_iter_t iter;
    int idx;
    rule_t rule;
    prefix_t p;
    int flag;

    for(i = 0; i < curr->ruleset.num; i ++) {
        r = curr->ruleset.ruleList[i].range[dim];
        iter = range_to_prefix_iter(r);
        do {
            p = get_prefix(&iter, dim_bits[dim]);
            if(p.len >= STRIDE)  {
                idx = (p.prefix >> (dim_bits[dim] - STRIDE)) + INL_OFFSET;
                n = next_exl(curr, idx - INL_OFFSET);

                p.len -= STRIDE;
                switch(dim_bits[dim]) {
                    case 8: 
                        p.prefix = (uint8_t)(p.prefix << STRIDE);
                        break;
                    case 16:
                        p.prefix = (uint16_t)(p.prefix << STRIDE);
                        break; 
                    case 32:
                        p.prefix = (uint32_t)(p.prefix << STRIDE);
                        break;
                }
            } else {
                idx = count_inl_bitmap(p.prefix >> (dim_bits[dim] - p.len), p.len); 
                n = next_inl(curr, idx);
                p.len = 0;
                p.prefix = 0;
            }

            memcpy(&rule, &curr->ruleset.ruleList[i], sizeof(rule_t));  
            prefix_to_range(&p, rule.range[dim], dim_bits[dim]);
            append_rules(&n->ruleset, &rule); 

            if(idx < INL_OFFSET) {
                push_inl_rules_even(idx, dim, curr, &rule, aux);
            }

            iter = get_next_prefix_iter(&iter, &flag); 
        }while(flag);
    }

    //bc_free(curr->ruleset.ruleList);
    //curr->ruleset.num = 0;
}

static inline int roundup_log2(int n)
{
    int i = 0;
    while(n) {
        n = (n-1)/2;
        i ++;
    }
    
    return i;
}

static
void even_cut_recursive(struct cnode *curr, int dim, \
                               struct cut_aux *aux)
{
    uint64_t mask = 0;
    int bit_pos;
    struct cnode *n;

    while(curr->mb.internal ^ mask) {
        bit_pos = __builtin_ctzll(curr->mb.internal ^ mask);
        n = next_inl(curr, bit_pos);
        remove_redund(&n->ruleset);
        cut_node(n, aux);
        set_bit(&mask, bit_pos);
    }

    mask = 0;
    while(curr->mb.external ^ mask) {
        bit_pos = __builtin_ctzll(curr->mb.external ^ mask);

        n = next_exl(curr, bit_pos);
        remove_redund(&n->ruleset);
        cut_node(n, aux);
        set_bit(&mask, bit_pos);
    }
}


int even_cut(struct cnode *n, int dim, struct cut_aux *aux)
{
    int i;
    int inl_rc = 0;
    int exl_rc = 0;
    for(i = 0; i < CHILDCOUNT; i++) {
        if(aux->hist[dim].child_rulecount[i]) {
            if(i < INL_OFFSET) {
                inl_rc ++;
                set_bit(&n->mb.internal, i);
            } else {
                exl_rc ++;
                set_bit(&n->mb.external, i - INL_OFFSET);
            }
        }
    }

    struct cnode *childs = bc_calloc(sizeof(struct cnode), (inl_rc+exl_rc));
    if(!childs) {
        PANIC("memory alloc fail\n");
    }
   
    n->mb.child_ptr = childs + inl_rc;

    push_rules_even(n, dim, aux);
    even_cut_recursive(n, dim, aux);
    return 0;
}

static int even_init(struct cnode *n, struct cut_aux *aux)
{
    return 0;
}

static int even_mem_quant(struct cut_aux *aux, int dim)
{
    return aux->hist[dim].childs;
}

static bool even_fits_bs(struct cut_aux *aux, int dim)
{
    int i;
    for(i = 0; i < CHILDCOUNT; i ++) {
        if(aux->hist[dim].child_rulecount[i]) {
            if(aux->hist[dim].child_rulecount[i] > BUCKETSIZE) {
                return false;
            }
        }
    }
    return true;
}
__attribute__((constructor)) static void register_cut_method(void)
{
    cuts[BITMAP_CUT].aux_init = even_init;
    cuts[BITMAP_CUT].cut_node = even_cut;
    cuts[BITMAP_CUT].match_expect = even_match_expect;
    cuts[BITMAP_CUT].mem_quant = even_mem_quant;
    cuts[BITMAP_CUT].all_fits_bs = even_fits_bs;
}


