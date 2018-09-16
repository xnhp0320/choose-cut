#include "cut-inl.h"
#include <stdbool.h>

struct cut_method cuts[MAX_CUT_TYPE];

void remove_redund(rule_set_t *ruleset)
{
    int i,j;

    for(i=1; i < ruleset->num; i++) {
        for(j=0; j < i; j++) {
            if(rule_contained(&ruleset->ruleList[j], \
                        &ruleset->ruleList[i])) {
                ruleset->ruleList[i].pri = UINT32_MAX;        
            }
        }
    }

    j = 1;
    for(i=1; i < ruleset->num; i ++) {
        if(ruleset->ruleList[i].pri != UINT32_MAX) {
            ruleset->ruleList[j++] = ruleset->ruleList[i];
        }
    }
    ruleset->num = j;
}

bool memory_constraints = true; 

double get_match_expect(struct rule_hist *hist)
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

int roundup_log2(int n)
{
    assert(n!= 0);
    int shift = 32 - __builtin_clz(n);

    if(n == (1UL << (shift -1)))
        return shift -1;
    
    return shift;
}

bool aux_heap_less(const void *a, const void *b)
{
    struct range1d *r1 = (struct range1d *)a;
    struct range1d *r2 = (struct range1d *)b;

    return r1->high < r2->high;
}

void
cut_recursive(struct cnode *cn, int childs, struct cut_aux *aux)
{
    int i;
    for(i = 0; i < childs; i++) {
        remove_redund(&cn[i].ruleset);
        cut_node(&cn[i], aux);
    }
}




