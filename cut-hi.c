#include "cut-inl.h"
#include "cut-hi.h"
#include "cut.h"
#include "mem.h"
#include "utils-inl.h"

static double
hi_get_me_1d(d_ranges_t ranges, struct range1d *bound, struct rule_hist *hist, uint32_t seg, \
                struct heap *h, uint8_t (node_r)[][2], int div)
{
    int i,j;
    uint64_t curr = (uint64_t)bound->low + seg;
    int childs = 0;
    int rules = 0;

    struct range1d *r, *top, *o;
    memset(hist, 0, sizeof(*hist));
    memset(node_r, 0, HICHILD *2 );

    r = &darray_item(ranges, 0);
    heap_push(h, r);
    i = 1;

    int prev_new_ends = 1;
    int new_start = 0, new_ends = 0;
    int new_child = 0;

    while(i < darray_size(ranges) || h->len != 0) {
        if(i < darray_size(ranges)) {
            r = &darray_item(ranges, i);
            if((uint64_t)(r->low) < curr) {
                heap_push(h, r);
                i++;
                new_start = 1;
                continue;
            }
        }

        while(h->len != 0 && \
                (uint64_t)((top = (struct range1d *)heap_peek(h))->high) < curr) { 
            rules += top->weight;
            heap_pop(h);
            new_ends = 1;
        }

        /* the left in the heap are ranges that r->low < curr but 
         * r->high >= curr, which are overlapped ranges */
        if(h->len != 0) {
            for(j=0; j < h->len; j++) {
                o = (struct range1d*)h->data[j];
                rules += o->weight;
            }
        }

        if(new_start) {
            new_child = 1;
        } else {
            /* if we do not have new start, we may have new ends
             * rule ending in this seg, means the next seg will 
             * not see this rule, so the next seg should be a 
             * new child, thus, whether this seg will generate 
             * a child or not dpends on the previous seg has 
             * new ends or not.
             *
             * The first seg will always generate a new child
             * as the boundary starts with one of the ranges' low
             * Thus, a rule new start event will always
             * happend in the first seg.
             */
            new_child = prev_new_ends;
        }
        prev_new_ends = new_ends;
        new_start = 0;
        new_ends = 0;

        if(new_child) {
            hist->child_rulecount[childs] = rules;
            hist->rules += rules;
            node_r[childs][0] = (childs == 0) ? 0 : node_r[childs-1][1] + 1;
            node_r[childs][1] = node_r[childs][0];
            childs ++;
        } else {
            node_r[childs - 1][1] ++;
        }

        rules = 0;
        curr += seg;
    }
    hist->childs = childs;

    /* in case the last segment is merged into the previous 
     * ones, where we do not have the chance to assign the 
     * ending segment number.
     */
    node_r[childs-1][1] = div - 1; 
    return get_match_expect(hist);
}

static double
calc_me_ratio(unsigned int rulenum, unsigned int dup_rules, double me)
{
    if(dup_rules != rulenum)
        return (rulenum - me) / (dup_rules - rulenum);
    return rulenum - me;
}

static double 
hi_get_rule_hist(rule_set_t *ruleset, int dim, struct cut_aux *cut_aux)
{
    int i;
    d_ranges_t ranges;
    struct hi_aux *aux = &cut_aux->hi_aux;
    struct rule_hist *hist = &aux->hist[dim];
    ranges = aux->ranges;

    for(i = 0; i < ruleset->num; i++) {
        darray_item(ranges, i).low = ruleset->ruleList[i].range[dim][0];
        darray_item(ranges, i).high = ruleset->ruleList[i].range[dim][1];
        darray_item(ranges, i).weight = 1;
    }

    qsort(ranges.item, ruleset->num, sizeof(struct range1d), _range_compare);
    int uniq_num = unique_ranges(ranges.item, ruleset->num);
    ranges.size = uniq_num;

    int div = 4;
    struct range1d bound = {.low = darray_item(ranges, 0).low, \
                             .high = darray_item(ranges, 0).high}; 
    
    for(i = 1; i < uniq_num; i++) {
        if(darray_item(ranges, i).high > bound.high) {
            bound.high = darray_item(ranges, i).high;
        }
    }

    uint64_t len = (uint64_t)bound.high - (uint64_t)bound.low + 1;
    /* if the length is too small, we should not continue  */       
    if(len < div) {
        return ruleset->num;
    }

    int shift = roundup_log2((len + div - 1)/div);
    double me;
    double prev_me;
    double me_ratio;
    double prev_me_ratio = 0.0;
    struct rule_hist tmp_hist;
    uint8_t node_r[HICHILD][2];

    me = hi_get_me_1d(ranges, &bound, &tmp_hist, (1ULL << shift), aux->h, node_r, div); 
    me_ratio = calc_me_ratio(ruleset->num, hist->rules, me); 

    aux->start[dim] = bound.low;

    // try 4, 8, 16, 32, 64
    do {
        aux->shift[dim] = shift;
        memcpy(aux->node_r[dim], node_r, HICHILD * 2); 
        aux->hist[dim] = tmp_hist;

        div*=2;
        shift = roundup_log2((len + div -1)/div);
        prev_me_ratio = me_ratio;
        prev_me = me;
        if(div > 64 || len < div || shift == aux->shift[dim]) break;

        me = hi_get_me_1d(ranges, &bound, &tmp_hist, (1ULL << shift), aux->h, node_r, div);
        me_ratio = calc_me_ratio(ruleset->num, hist->rules, me);
    } while(div<=64 && me_ratio > prev_me_ratio);

    return prev_me; 
}

static double hi_match_expect(struct cnode *n, int dim, struct cut_aux *aux)
{
    if(n == 0x7ffff5a8dcd0)
        LOG("HELLO");

    return hi_get_rule_hist(&n->ruleset, dim, aux);
}

static bool
hi_fits_bs(struct cut_aux *aux, int dim)
{
    int i;
    for(i = 0; i < CHILDCOUNT; i ++ ) {
        if(aux->hi_aux.hist[dim].child_rulecount[i]) {
            if(aux->hi_aux.hist[dim].child_rulecount[i] > BUCKETSIZE) {
                return false;
            }
        }
    }
    return true;
}

static void
hi_traverse(struct cnode *curr, void (*traverse_func)(struct cnode *n, void *arg, int depth), void *arg, int depth)
{
    uint64_t mask = 0;
    struct cnode *n;
    int bit_pos;

    while(curr->hn.bitmap ^ mask) {
        bit_pos = __builtin_ctz(curr->hn.bitmap ^ mask);
        n = hi_next(curr, bit_pos); 
        traverse(n, traverse_func, arg, depth + 1);
        mask |= (1 << bit_pos);
    }
}

static int 
hi_init(struct cnode *n, struct cut_aux* aux)
{
    darray_init(aux->hi_aux.ranges);
    darray_growalloc(aux->hi_aux.ranges, n->ruleset.num);

    aux->hi_aux.h = heap_init(aux_heap_less);

    return 0;
}

static int
hi_mem_quant(struct cut_aux *aux, int dim)
{
    return aux->hi_aux.hist[dim].childs;
}


static int
hi_count_childs(struct cnode *n)
{
    if(!n->leaf)
        return __builtin_popcount(n->hn.bitmap);
    return 0;
}

static void
hi_set_child_ptr(struct cnode *n, struct cnode *c, struct cnode *childs)
{
    c->hn.child_ptr = childs;
}

static void
hi_push_rule(rule_t *r, int dim, struct cnode *childs, struct hi_aux *aux)
{
    unsigned int s,e;
    s = ((r->range[dim][0] - aux->start[dim]) >> aux->shift[dim]); 
    e = ((r->range[dim][1] - aux->start[dim]) >> aux->shift[dim]);

    int cs = -1, ce = -1;
    int i;

    for(i = 0; i < aux->hist[dim].childs; i ++) {
        if(s >= aux->node_r[dim][i][0] && \
                s <= aux->node_r[dim][i][1] && cs == -1) {
            cs = i;
        }

        if(e >= aux->node_r[dim][i][0] && \
                e <= aux->node_r[dim][i][1] && ce == -1) {
            ce = i;
        }

        if(cs != -1 && ce != -1)
            break;
    }

    assert(cs <= ce);

    uint64_t trim_s, trim_e;

    for(i = cs; i <= ce ; i++) {
        rule_t rt = *r;
        trim_s = trim_e = aux->start[dim];
        trim_s += aux->node_r[dim][i][0] * (1ULL << aux->shift[dim]);
        trim_e += (aux->node_r[dim][i][1] + 1) * (1ULL << aux->shift[dim]);

        /* trim the range */
        if(rt.range[dim][0] < trim_s)
            rt.range[dim][0] = (uint32_t)trim_s;

        if(rt.range[dim][1] >= trim_e)
            rt.range[dim][1] = (uint32_t)(trim_e - 1);

        append_rules(&childs[i].ruleset, &rt);
    }
}

static void
hi_set_bitmap(struct cnode *n, int dim, struct hi_aux *aux)
{
    /* the bitmap is a like an array storing the XOR of 
     * the bit indicating whether the adjacent segment contains new rules or not. If
     * two adjacent segments are the same, the xor equals 0, if 
     * two ajacent segments are different, the xor equals 1. 
     * eg. [0, 1] [2, 3] will generate the bitmap: 010, the 
     * first two has 0, the 2nd and 3rd generate 1, the 3rd and 
     * the 4th generate 0. 
     * For N childs, we only have N-1 bits
     */
    int i;
    for(i = 0; i < aux->hist[dim].childs - 1; i ++) {
        set_bit(&n->hn.bitmap, aux->node_r[dim][i][1]);
    }
}

static void
hi_set_node(struct cnode *n, int dim, struct hi_aux *aux)
{
    hi_set_bitmap(n, dim, aux);
    n->hn.start = aux->start[dim];
    n->hn.shift = aux->shift[dim];
}

static int hi_cut(struct cnode *n, int dim, struct cut_aux *cut_aux)
{
    if(n == 0x7ffff5a8dcd0) 
        LOG("HELLO");

    struct hi_aux *aux = &cut_aux->hi_aux;
    struct cnode *childs = bc_calloc(aux->hist[dim].childs, sizeof(*n));
    if(!childs) {
        PANIC("Memory alloc fail\n");
    }
    n->hn.child_ptr = childs;

    int i;
    for(i = 0; i < aux->hist[dim].childs; i ++) {
        rule_set_init(&childs[i].ruleset, aux->hist[dim].child_rulecount[i]);
    }

    rule_t *r;
    for(i = 0; i < n->ruleset.num; i ++ ) {
        r = &n->ruleset.ruleList[i];
        hi_push_rule(r, dim, childs, aux); 
    }

    for(i = 0; i < aux->hist[dim].childs; i ++ ) {
        assert(childs[i].ruleset.num == aux->hist[dim].child_rulecount[i]);
    }

    hi_set_node(n, dim, aux);

    rule_set_free(&n->ruleset);
    cut_recursive(childs, aux->hist[dim].childs, cut_aux);

    return 0;
}

__attribute__((constructor)) static void register_cut_method(void)
{
    cuts[HI_CUT].aux_init = hi_init;
    cuts[HI_CUT].cut_node = hi_cut;
    cuts[HI_CUT].match_expect = hi_match_expect;
    cuts[HI_CUT].mem_quant = hi_mem_quant;
    cuts[HI_CUT].all_fits_bs = hi_fits_bs;
    cuts[HI_CUT].traverse = hi_traverse;
    cuts[HI_CUT].count_childs = hi_count_childs;
    cuts[HI_CUT].set_child_ptr = hi_set_child_ptr;
}

