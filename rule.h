#ifndef __RULE_H__
#define __RULE_H__

#include <stdio.h>
#include <stdint.h>
#include "param.h"

typedef struct prefix
{
    unsigned int prefix;
    unsigned int len;
}prefix_t;

typedef struct prefix_iter
{
    uint32_t start;
    uint32_t end;
    uint32_t _end;
    uint64_t _left;
} prefix_iter_t;


typedef	struct rule_s
{
	unsigned int	pri;
	unsigned int	range[DIM][2];
} rule_t;

typedef struct rule_set_s
{
	unsigned int	num; /* number of rules in the rule set */
	unsigned int	cap; /* number of rules in the rule set */
	rule_t*			ruleList; /* rules in the set */
} rule_set_t;

struct flow {
    uint32_t key[DIM];
};

typedef unsigned int range_t;

struct range1d {
    unsigned int low;
    unsigned int high;
    unsigned int weight;
};

/*-----------------------------------------------------------------------------
 *  function declaration
 *-----------------------------------------------------------------------------*/


int rule_contained(rule_t *a, rule_t *b);
void show_ruleset(rule_set_t *ruleset);
int _rule_pri_compare(const void *a, const void *b);

/* ruleset management */
int rule_set_init(rule_set_t *ruleset, int num);
void rule_set_free(rule_set_t *ruleset);

prefix_iter_t range_to_prefix_iter(range_t *range);
prefix_iter_t get_next_prefix_iter(prefix_iter_t *curr, int *flag);

static inline int is_empty_prefix_iter(prefix_iter_t *iter)
{
    return iter->_left == 0;
}
prefix_t get_prefix(prefix_iter_t *iter, int bits);
#define HS_RULE_NOT_FOUND -1 

static inline void prefix_to_range(prefix_t *p, range_t *r, int bits)
{
    r[0] = p->prefix;
    r[1] = p->prefix + ((1ULL << (bits - p->len)) -1);
}

static inline int range_overlap(range_t *r1, range_t *r2)
{
    if((r1[0] > r2[1]) || (r1[1] < r2[0]))
        return 0;

    return 1;
}


static inline void range_to_prefix(prefix_t *p, range_t *r, int bits)
{
    uint64_t offset = r[1] - r[0];
    p->len  = bits - __builtin_ctzl(offset + 1);
    p->prefix = r[0];
}
int rule_set_copy(rule_set_t *dest, rule_set_t *src);
int unique_ranges(struct range1d *ranges, int num);
int _range_compare(const void *a, const void *b);

#endif
