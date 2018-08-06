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

bool memory_constraints = false; 
