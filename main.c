#include <ccan/opt/opt.h>
#include "mb_node.h"
#include "utils-inl.h"
#include "rule.h"
#include "cut.h"
#include <stdarg.h>
#include <time.h>


#ifndef __MACH__
#define __MACH__ 0
#endif

#if __MACH__
#include <mach/mach_time.h>
#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)

static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;

struct timespec orwl_gettime(void) {
    // be more careful in a multithreaded environement
    if (!orwl_timestart) {
        mach_timebase_info_data_t tb = { 0 };
        mach_timebase_info(&tb);
        orwl_timebase = tb.numer;
        orwl_timebase /= tb.denom;
        orwl_timestart = mach_absolute_time();
    }
    struct timespec t;
    double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
    t.tv_sec = diff * ORWL_NANO;
    t.tv_nsec = diff - (t.tv_sec * ORWL_GIGA);
    return t;
}
#define CLOCK_GETTIME(x) *(x)=orwl_gettime()
#else
#define CLOCK_GETTIME(x) clock_gettime(CLOCK_MONOTONIC, x);
#endif



static const char *filename;
static char *rule_file(const char *optarg, void *unused)
{
    filename = optarg;
    printf("filename: %s\n", filename);
    return NULL;
}

static void err_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	/* Check return, for fascist gcc */
    vprintf(fmt, ap);
	va_end(ap);
    vprintf("\n", ap);
}

static
struct flow *sample_rules(rule_set_t *ruleset, int samples_cnt)
{
    struct flow *flow = calloc(ruleset->num, samples_cnt * sizeof(struct flow));
    int i, j, k;
    for(i = 0; i < ruleset->num; i ++) {
        rule_t * r = &(ruleset->ruleList[i]);
        for(j = 0; j < samples_cnt; j ++) {
            for(k = 0; k < DIM; k ++) {
                flow[i * samples_cnt + j].key[k] = \
                    rand() % ((unsigned long)r->range[k][1] - r->range[k][0] + 1ULL) + r->range[k][0];
            }
        }
    }
    return flow;
}

static
void sample_testing(struct cnode *root, rule_set_t *ruleset)
{
    int sample_cnt = 100;
    int i;
    int pri;

    struct flow *f = sample_rules(ruleset, sample_cnt);

    for(i = 0; i < ruleset->num * sample_cnt; i++) {
        if(i == 47028)
            LOG("HELLO");

        int lpri = lsearch(ruleset, &f[i]);
        pri = search(root, &f[i]);  

        if(pri != lpri) {
            printf("error sample testing, pri %d lpri %d rule %d i %d\n", pri, lpri, i/100, i);
        }
    }
}

static
void sample_perf(struct cnode *root, rule_set_t *ruleset)
{
    struct timespec tp_b;
    struct timespec tp_a;
    int sample_cnt = 100;
    int i;
    int pri;

    struct flow *f = sample_rules(ruleset, sample_cnt);

    CLOCK_GETTIME(&tp_b);
    for(i = 0; i < ruleset->num * sample_cnt; i++) {
        pri = search(root, &f[i]);  
        pri++;
    }
    CLOCK_GETTIME(&tp_a);

    long nano = (tp_a.tv_nsec > tp_b.tv_nsec) ? (tp_a.tv_nsec -tp_b.tv_nsec) : (tp_a.tv_nsec - tp_b.tv_nsec + 1000000000ULL);
    printf("sec %ld, nano %ld\n", tp_b.tv_sec, tp_b.tv_nsec);
    printf("sec %ld, nano %ld\n", tp_a.tv_sec, tp_a.tv_nsec);
    printf("nano %ld us %ld ms %ld\n", nano, nano/1000, nano/1000000UL);
    printf("speed %.2fMpps\n", (1e9/((double)nano/(ruleset->num * sample_cnt)))/1e6);
}



int main(int argc, char *argv[])
{
    int ret;
    opt_register_arg("-f", rule_file, NULL, NULL, "");
    ret = opt_parse(&argc, argv, err_printf);
    if(!ret) {
        exit(-1);
    } 

    if(filename == NULL) {
        exit(-1);
    }
    
    rule_set_t ruleset;
    ReadFilterFile(&ruleset, filename);

    struct cnode root;
    memset(&root, 0, sizeof(root));
    rule_set_copy(&root.ruleset, &ruleset);
    cut(&root);
    get_tree_info(&root);

    ctree_t ct = compact(&root);
    struct cnode *croot = &darray_item(ct, 0);

    sample_testing(&root, &ruleset);
    sample_perf(croot, &ruleset);
    return 0;
}
