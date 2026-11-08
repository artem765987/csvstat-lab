#ifndef STATS_H
#define STATS_H

#include "table.h"

void stats_reset(struct Column *column);
void stats_add_int(struct Column *column, long long value,
                   unsigned long long rows, int first);
void stats_add_double(struct Column *column, double value,
                      unsigned long long rows, int first);
void stats_print_int_mean(const struct Column *column,
                          unsigned long long rows);

void print_schema(const struct Table *table);
void print_stats(const struct Table *table);

#endif
