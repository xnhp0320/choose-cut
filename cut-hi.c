#include "cut-inl.h"
#include "cut-hi.h"
#include "cut.h"

static double
hi_get_me_1d(d_ranges_t ranges, struct range1d *bound, struct rule_hist *hist, uint32_t seg, struct heap *h)
{
    int i,j;
    uint64_t curr = (uint64_t)bound->low + seg;
    int childs = 0;
    int rules = 0;

    struct range1d *r, *top, *o;
    memset(hist, 0, sizeof(*hist));

    r = &darray_item(ranges, 0);
    heap_push(h, r);
    i = 1;
    int new_flag;

    while(i < darray_size(ranges) || h->len != 0) {
        if(i < darray_size(ranges)) {
            r = &darray_item(ranges, i);
            if((uint64_t)(r->low) < curr) {
                heap_push(h, r);
                i++;
                new_flag = 1;
                continue;
            }
        }

        while(h->len != 0 && \
                (uint64_t)((top = (struct range1d *)heap_peek(h))->high) < curr) { 
            rules += top->weight;
            heap_pop(h);
        }

        /* the left in the heap are ranges that r->low < curr but 
         * r->high >= curr, which are overlapped ranges */
        if(h->len != 0) {
            for(j=0; j < h->len; j++) {
                o = (struct range1d*)h->data[j];
                rules += o->weight;
            }
        }

        if(new_flag) {
            hist->child_rulecount[childs] = rules;
            hist->rules += rules;
            childs ++;
            new_flag = 0;
        }

        rules = 0;
        curr += seg;
    }
    hist->childs = childs;

    return get_match_expect(hist);
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
    int shift = roundup_log2(len/div);

    double me;
    double prev_me = 0.0;

    me = hi_get_me_1d(ranges, &bound, hist, (1ULL << shift), aux->h); 
    aux->start[dim] = bound.low;

    // try 4, 8, 16, 32, 64
    do {
        aux->shift[dim] = shift;

        shift -= 1;
        prev_me = me;
        div*=2;
        if(div > 64) break;

        me = hi_get_me_1d(ranges, &bound, hist, (1ULL << shift), aux->h);
    } while(div<=64 && me < prev_me && me - prev_me < 1);


    return me; 
}

static double hi_match_expect(struct cnode *n, int dim, struct cut_aux *aux)
{
    //if(n == 0x858300)
    //    LOG("HELLO");

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
    uint8_t mask = 0;
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

static int hi_cut(struct cnode *n, int dim, struct cut_aux *cut_aux)
{
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

