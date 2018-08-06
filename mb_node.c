#include <stdio.h>
#include "mb_node.h"
#include "mm.h"

#ifdef FAST_TREE_FUNCTION
static uint64_t fct[1<<(STRIDE - 1)];

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
            stride >>= 1;
        }
    }
}

int tree_function(uint64_t bitmap, uint8_t stride)
{
    uint64_t ret;
    int pos;

    ret = fct[(stride>>1)] & bitmap;
    if(ret){
        pos = __builtin_clzll(ret);
        return BITMAP_BITS - 1 - pos;
    }
    else
        return -1;
}

#else

int tree_function(uint64_t bitmap, uint8_t stride)
{
    int i;
    int pos;
    if (bitmap == 0ULL)
        return -1;
    for(i=STRIDE-1;i>=0;i--){
        stride >>= 1;
        pos = count_inl_bitmap(stride, i); 
        if (test_bitmap(bitmap, pos)){
            return pos;
        }
    }
    return -1;
}
#endif

