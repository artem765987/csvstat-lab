#include "stats.h"
#include <stdio.h>
#include <math.h>

void stats_reset(struct Column *column)
{
    column->int_min = 0;
    column->int_max = 0;
    column->quotient_sum = 0;
    column->remainder_sum = 0;
    column->double_min = 0.0;
    column->double_max = 0.0;
    column->double_mean = 0.0;
    column->positive = 0;
    column->negative = 0;
    column->even = 0;
    column->integer = 0;
}

void stats_add_int(struct Column *column, long long value,
                   unsigned long long rows, int first)
{
    if (first || value < column->int_min) {
        column->int_min = value;
    }
    if (first || value > column->int_max) {
        column->int_max = value;
    }
    if (value > 0) {
        column->positive++;
    }
    if (value < 0) {
        column->negative++;
    }
    if (value % 2 == 0) {
        column->even++;
    }

    column->quotient_sum += value / (long long)rows;
    column->remainder_sum += value % (long long)rows;
}

void stats_add_double(struct Column *column, double value,
                      unsigned long long rows, int first)
{
    if (first || value < column->double_min) {
        column->double_min = value;
    }
    if (first || value > column->double_max) {
        column->double_max = value;
    }
    if (value > 0.0) {
        column->positive++;
    }
    if (value < 0.0) {
        column->negative++;
    }
    if (floor(value) == value) {
        column->integer++;
    }

    column->double_mean += value / (double)rows;
}

void stats_print_int_mean(const struct Column *column,
                          unsigned long long rows)
{
    long long whole = column->quotient_sum;
    long long part = column->remainder_sum;
    unsigned long long magnitude;
    unsigned long long fraction;
    int negative;

    whole += part / (long long)rows;
    part %= (long long)rows;

    if (whole > 0 && part < 0) {
        whole--;
        part += (long long)rows;
    } else if (whole < 0 && part > 0) {
        whole++;
        part -= (long long)rows;
    }

    negative = whole < 0 || (whole == 0 && part < 0);

    if (whole < 0) {
        magnitude = (unsigned long long)(-(whole + 1)) + 1;
    } else {
        magnitude = (unsigned long long)whole;
    }
    if (part < 0) {
        fraction = (unsigned long long)(-part);
    } else {
        fraction = (unsigned long long)part;
    }

    fraction = (fraction * 1000000ULL + rows / 2) / rows;
    if (fraction == 1000000ULL) {
        magnitude++;
        fraction = 0;
    }
    if (magnitude == 0 && fraction == 0) {
        negative = 0;
    }

    printf("%s%llu.%06llu\n", negative ? "-" : "", magnitude, fraction);
}

static const char *type_name(int type)
{
    if (type == TYPE_INT) {
        return "INT";
    }
    return "DOUBLE";
}

static void print_real(double value)
{
    if (value > -0.0000005 && value < 0.0000005) {
        value = 0.0;
    }
    printf("%.6f\n", value);
}

void print_schema(const struct Table *table)
{
    unsigned int i;

    printf("rows %llu\n", table->rows);
    printf("columns %u\n", table->count);
    for (i = 0; i < table->count; i++) {
        printf("%u \"%s\" %s\n", i, table->columns[i].name,
               type_name(table->columns[i].type));
    }
}

void print_stats(const struct Table *table)
{
    unsigned int i;
    const struct Column *column;

    printf("rows %llu\n", table->rows);
    printf("columns %u\n", table->count);

    for (i = 0; i < table->count; i++) {
        column = &table->columns[i];
        if (i != 0) {
            printf("\n");
        }
        printf("column %u\n", i);
        printf("name \"%s\"\n", column->name);
        printf("type %s\n", type_name(column->type));
        printf("nulls 0\n");

        if (column->type == TYPE_INT) {
            printf("min %lld\n", column->int_min);
            printf("max %lld\n", column->int_max);
            printf("mean ");
            stats_print_int_mean(column, table->rows);
            printf("positive %llu\n", column->positive);
            printf("negative %llu\n", column->negative);
            printf("even %llu\n", column->even);
        } else {
            printf("min ");
            print_real(column->double_min);
            printf("max ");
            print_real(column->double_max);
            printf("mean ");
            print_real(column->double_mean);
            printf("positive %llu\n", column->positive);
            printf("negative %llu\n", column->negative);
            printf("integer %llu\n", column->integer);
        }
    }
}
