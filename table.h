#ifndef TABLE_H
#define TABLE_H

#include <stdio.h>

#define TYPE_INT 1
#define TYPE_DOUBLE 2

struct Column {
    char *name;
    int type;

    long long int_min;
    long long int_max;
    long long quotient_sum;
    long long remainder_sum;

    double double_min;
    double double_max;
    double double_mean;

    unsigned long long positive;
    unsigned long long negative;
    unsigned long long even;
    unsigned long long integer;
};

struct Table {
    struct Column *columns;
    unsigned int count;
    unsigned long long rows;
};

void table_init(struct Table *table);
void table_free(struct Table *table);
int table_read(FILE *file, struct Table *table);

#endif
