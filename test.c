#include "rule.h"
#include <assert.h>

#include "cut.h"
#include "utils-inl.h"

static int test1(void)
{
    unsigned int range[2] = {1,4};
    prefix_iter_t iter = range_to_prefix_iter(range);

    assert(iter.start == iter.end);
    assert(iter.start == 1);
    int count = 0;

    do {
        iter = get_next_prefix_iter(&iter);
        count ++;
    }while(!is_empty_prefix_iter(&iter));

    assert(count == 2);
    return 0;
}

static int test2(void)
{
    unsigned int range[2] = {0, (1ULL << 32) -1};
    prefix_iter_t iter = range_to_prefix_iter(range);

    assert(iter.start == 0);
    assert(iter.end == (1ULL << 32) -1);
    int count = 0;

    do {
        iter = get_next_prefix_iter(&iter);
        if(!is_empty_prefix_iter(&iter))
            count ++;
    }while(!is_empty_prefix_iter(&iter));

    assert(count == 0);
    return 0;
}

static int test3(void)
{
    unsigned int range[2] = {1, (1ULL << 32) -1};
    printf("[%u, %u] expands to:\n", range[0], range[1]);
    prefix_iter_t iter = range_to_prefix_iter(range);
    printf("[%x, %x]\n", iter.start, iter.end);
    assert(iter.start == 1);
    assert(iter.start == iter.end);
    int count = 0;

    do {
        iter = get_next_prefix_iter(&iter);
        printf("[%x, %x]\n", iter.start, iter.end);
        count ++;
    }while(!is_empty_prefix_iter(&iter));
    assert(count == 31);
    printf("\n");

    return 0;
}

static int test4(void)
{
    unsigned int range[2] = {1, (1ULL << 32)-2};
    printf("[%u, %u] expands to:\n", range[0], range[1]);
    prefix_iter_t iter = range_to_prefix_iter(range);
    printf("[%x, %x]\n", iter.start, iter.end);
    assert(iter.start == 1);
    assert(iter.start == iter.end);
    int count = 0;

    do {
        iter = get_next_prefix_iter(&iter);
        printf("[%x, %x]\n", iter.start, iter.end);
        count ++;
    }while(!is_empty_prefix_iter(&iter));
    assert(count == 61);
    printf("\n");

    return 0;
}


int main(int argc, char *argv[])
{
    test1();
    test2();
    test3();
    test4();
    return 0;
}
