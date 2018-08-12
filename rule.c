#include "rule.h"
#include <string.h>
#include "mem.h"

int rule_set_init(rule_set_t *ruleset, int num)
{
    ruleset->ruleList = bc_calloc(num, sizeof(rule_t));
    if(!ruleset->ruleList) {
        return -1;
    }
    ruleset->num = 0;
    ruleset->cap = num;
    return 0;
}

void rule_set_free(rule_set_t *ruleset)
{
    ruleset->num = 0;
    ruleset->cap = 0;
    bc_free(ruleset->ruleList);
    ruleset->ruleList = NULL;
}

int rule_set_copy(rule_set_t *dest, rule_set_t *src)
{
    dest->ruleList = bc_malloc(src->num * sizeof(rule_t));
    if(!dest->ruleList)
        return -1;

    dest->num = src->num;
    dest->cap = src->num;
    memcpy(dest->ruleList, src->ruleList, src->num * sizeof(rule_t));
    return 0;
}

prefix_iter_t range_to_prefix_iter(range_t *range)
{
    prefix_iter_t iter;
    iter.start = range[0];
    iter._end = range[1];
    iter._left = (uint64_t)range[1] - range[0] + 1; 

    if(iter.start & 0x1) {
        iter.end = iter.start;
        iter._left --;
        return iter;
    }

    int left_leap = 63 - __builtin_clzl(iter._left);
    int self_leap = __builtin_ctz(iter.start); 
    uint64_t offset = (1ULL << (left_leap < self_leap ? left_leap : self_leap));
    iter.end = iter.start + (uint32_t)(offset - 1);
    iter._left -= offset;

    return iter;
}


prefix_iter_t get_next_prefix_iter(prefix_iter_t *curr, int *flag)
{
    prefix_iter_t iter = {0};
    if(is_empty_prefix_iter(curr)) {
        *flag = 0;
        return iter;
    }
    *flag = 1;

    iter.start = curr->end + 1;
    iter._end = curr->_end;
    iter._left = curr->_left;
    if(iter.start & 1) {
        iter.end = iter.start;
        iter._left --;
        return iter;
    }

    int left_leap = 63 - __builtin_clzl(iter._left);
    int self_leap = __builtin_ctz(iter.start); 
    uint64_t offset = (1ULL << (left_leap < self_leap ? left_leap : self_leap));
    iter.end = iter.start + (uint32_t)(offset - 1);
    iter._left -= offset;

    return iter;
}

prefix_t get_prefix(prefix_iter_t *iter, int bits)
{
    prefix_t p;
    uint64_t offset = iter->end - iter->start;
    if(offset == 0) {
        p.len = bits;
        p.prefix = iter->start;
        return p;
    }

    p.len  = bits - __builtin_ctzl(offset + 1);
    p.prefix = iter->start;
    return p;
}



