#include "cut.h"
#include "utils-inl.h"
#include "mem.h"

static int node_cnt;
static int leaf_cnt;
static int rule_cnt;

void cut_node(struct cnode *n, struct cut_aux *aux)
{
    node_cnt ++;

    if(n->ruleset.num <= BUCKETSIZE) {
        n->leaf = 1;
        leaf_cnt++;
        rule_cnt += n->ruleset.num;
        return;
    }
    
    int i;
    int t;
    double min_t, min[MAX_CUT_TYPE], me[MAX_CUT_TYPE];
    int chose_dim[MAX_CUT_TYPE];
    int chose_t;
   
    for(t = 0; t < MAX_CUT_TYPE; t ++ ) {
        for(i = 0; i < DIM; i ++) {
            me[t] = cuts[t].match_expect(n, i, aux);
            if(i == 0) {
                min[t] = me[t];
                chose_dim[t] = i;
            } else {
                if(me[t] < min[t]) {
                    min[t] = me[t];
                    chose_dim[t] = i;
                }
            }
        }
        //LOG("for cut %d, min %.2f, chose_dim %d\n", t, min[t], chose_dim[t]);
    }

    chose_t = 0;
    min_t = (double)n->ruleset.num;

    for(t = 0; t < MAX_CUT_TYPE; t ++) {
        if(min[t] < min_t) {
            min_t = min[t];
            chose_t = t;
        }
    }

    if(memory_constraints) {
        int mem_quant[MAX_CUT_TYPE];
        for(t = 0; t < MAX_CUT_TYPE; t++) {
            if(cuts[t].all_fits_bs(aux, chose_dim[t])) {
                mem_quant[t] = cuts[t].mem_quant(aux, chose_dim[t]); 
            } else {
                mem_quant[t] = CHILDCOUNT;
            }
        }

        for(t = 0; t < MAX_CUT_TYPE; t++ ) {
            if(mem_quant[t] < mem_quant[0]) {
                mem_quant[0] = mem_quant[t];
                chose_t = t;
            }
        }
    }


    //LOG("Choose %d Cut method %d\n", chose_dim[chose_t], chose_t);    
    n->dim = chose_dim[chose_t];
    cuts[chose_t].cut_node(n, chose_dim[chose_t], aux);
}

void cut(struct cnode *root)
{
    struct cut_aux aux;
    int t;
    for(t = 0; t < MAX_CUT_TYPE; t ++ )
        cuts[t].aux_init(root, &aux);

    cut_node(root, &aux);
    LOG("NODE %d\n", node_cnt);
    LOG("LEAF %d\n", leaf_cnt);
    LOG("RULES %d\n", rule_cnt);
    
}


