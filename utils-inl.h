#ifndef __UTILS_INL_H__
#define __UTILS_INL_H__
#include "rule.h"
#include <stdio.h>
#include <stdbool.h>

#define MAXFILTERS	65535 /* support 64K rules */	
struct FILTER						
{
    unsigned int cost;		
    unsigned int dim[DIM][2];
    unsigned char act;	
};

struct FILTSET
{
    unsigned int	numFilters;	
    struct FILTER	filtArr[MAXFILTERS];
};


/* read rules from file */
int	 ReadFilterFile(rule_set_t* ruleset, const char* filename); /* main */
void LoadFilters(FILE* fp, struct FILTSET* filtset);
int	 ReadFilter(FILE* fp, struct FILTSET* filtset,	unsigned int cost);
void ReadIPRange(FILE* fp, unsigned int* IPrange);
void ReadPort(FILE* fp, unsigned int* from, unsigned int* to);
void ReadPri(FILE *fp, unsigned int *cost);
void ReadProtocol(FILE* fp, unsigned int* from, unsigned int* to);


#define PANIC(format, msg...) \
    do { \
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);\
        perror(format, ##msg); \
        exit(-1); \
    }while(0)

#define LOG(format, msg...)\
    do {\
        printf(format, ##msg); \
    }while(0)

#endif

bool double_equals(double a, double b);
