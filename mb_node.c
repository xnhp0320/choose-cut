#include <stdio.h>
#include "mb_node.h"
#include "mm.h"

#ifdef FAST_TREE_FUNCTION
uint64_t fct[1<<(STRIDE - 1)];
int pos2cidr[1 << STRIDE];

__attribute__((constructor)) void fast_lookup_init(void)
{
    uint32_t i;
    int j;
    int pos;

    uint32_t stride;
    for(i=0;i<(1<<(STRIDE-1));i++) {
        stride = i;
        for(j = STRIDE -1; j>=0; j--) {
            pos = count_inl_bitmap(stride, j);
            set_bit(fct+i, pos);
            pos2cidr[pos] = j + 1;
            stride >>= 1;
        }
    }
}
#endif


