#ifndef _CUT_INL_H_
#define _CUT_INL_H_

#include "mb_node.h"
#include "rule.h"
#include <ccan/heap/heap.h>
#include <ccan/darray/darray.h>

#define CHILDCOUNT ((1<<STRIDE) + (1<<STRIDE) - 1)
#define INL_OFFSET ((1<< STRIDE) -1)
#define HICHILD ((1<<STRIDE))
struct rule_hist{
    int child_rulecount[CHILDCOUNT];
    int childs;
    int rules;
};

struct sp_node {
    uint8_t dim_left;
    uint8_t dim_right;
    uint8_t shape;
    uint8_t pad;

    uint32_t root_split;
    uint32_t left_split;
    uint32_t right_split;

    void *child_ptr;
};

struct hi_node {
    uint32_t start;
    uint32_t shift;
    uint64_t bitmap;

    void * child_ptr;
};

struct cnode {
    union {
        struct mb_node mb;
        struct sp_node sp;
        struct hi_node hn;
    };

    union{
        uint32_t control;
        struct {
            uint32_t dim:3;
            uint32_t leaf:1;
            uint32_t type:2;
            uint32_t pad:26;
        };
    };
    rule_set_t ruleset;
};

struct snode {
    unsigned int dim; 
    unsigned int flags;
    struct range1d r[2];
    rule_set_t ruleset;
};

struct stree {
    struct snode root;
    struct snode left;
    struct snode right;
};

typedef darray(struct range1d) d_ranges_t;

struct split_aux {
    d_ranges_t ranges;
    struct heap *h; 
    rule_set_t left_rules;
    rule_set_t right_rules;
    struct stree strees[DIM];
};

struct hi_aux {
    d_ranges_t ranges;
    struct heap *h;
    struct rule_hist hist[DIM];
    uint32_t shift[DIM];
    uint32_t start[DIM];
    uint8_t  node_r[DIM][HICHILD][2];
};

struct cut_aux {
    /* bitmap aux */
    struct rule_hist hist[DIM];
    /* split aux */
    struct split_aux split_aux; 
    /* hi cut*/
    struct hi_aux hi_aux;
};

void remove_redund(rule_set_t *ruleset);

static inline void append_rules(rule_set_t *ruleset, rule_t *r)
{
    assert(ruleset->num < ruleset->cap);
    ruleset->ruleList[ruleset->num ++] = *r;
}

struct cut_method {
    int (*aux_init)(struct cnode *n, struct cut_aux *aux);
    int (*cut_node)(struct cnode *n, int dim, struct cut_aux *aux);
    double (*match_expect)(struct cnode *n, int dim, struct cut_aux *aux);
    int (*mem_quant)(struct cut_aux *aux, int dim);
    bool (*all_fits_bs)(struct cut_aux *aux, int dim);
    void (*traverse)(struct cnode *node, void (*traverse_func)(struct cnode *n, void *arg, int depth), void *arg, int depth);
    int (*count_childs)(struct cnode *n);
    void (*set_child_ptr)(struct cnode *n, struct cnode *c, struct cnode *childs);
};

enum cut_type {
    BITMAP_CUT,
    SPLIT_CUT,
    HI_CUT,
    MAX_CUT_TYPE
};

extern struct cut_method cuts[MAX_CUT_TYPE];
void cut_node(struct cnode *n, struct cut_aux *aux);

extern bool memory_constraints;
double get_match_expect(struct rule_hist *hist);

int roundup_log2(int n);
bool aux_heap_less(const void *a, const void *b);

void
cut_recursive(struct cnode *cn, int childs, struct cut_aux *aux);
#endif
