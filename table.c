#include "table.h"
#include "stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void table_init(struct Table *table)
{
    table->columns = NULL;
    table->count = 0;
    table->rows = 0;
}

void table_free(struct Table *table)
{
    unsigned int i;
    for (i = 0; i < table->count; i++) {
        free(table->columns[i].name);
    }
    free(table->columns);
    table_init(table);
}

static int read_field(FILE *file, char **buffer, size_t *capacity)
{
    size_t length = 0;
    size_t new_capacity;
    char *temporary;
    int c;

    while (1) {
        c = fgetc(file);
        if (c == ',' || c == '\n' || c == EOF) {
            break;
        }
        if (length + 1 >= *capacity) {
            new_capacity = *capacity == 0 ? 32 : *capacity * 2;
            if (new_capacity <= *capacity) {
                return -2;
            }
            temporary = realloc(*buffer, new_capacity);
            if (temporary == NULL) {
                return -2;
            }
            *buffer = temporary;
            *capacity = new_capacity;
        }
        (*buffer)[length] = (char)c;
        length++;
    }

    if (length + 1 >= *capacity) {
        new_capacity = length + 2;
        temporary = realloc(*buffer, new_capacity);
        if (temporary == NULL) {
            return -2;
        }
        *buffer = temporary;
        *capacity = new_capacity;
    }
    (*buffer)[length] = '\0';
    if (c == EOF && ferror(file)) {
        return -2;
    }
    return c;
}

static int parse_int(const char *text, long long *result)
{
    unsigned long long value = 0;
    unsigned long long limit = 9223372036854775807ULL;
    unsigned int i = 0;
    unsigned int digit;
    int negative = 0;

    if (text[i] == '+' || text[i] == '-') {
        negative = text[i] == '-';
        i++;
    }
    if (negative) {
        limit++;
    }
    if (text[i] == '\0') {
        return 0;
    }

    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        digit = (unsigned int)(text[i] - '0');
        if (value > (limit - digit) / 10) {
            return 0;
        }
        value = value * 10 + digit;
        i++;
    }

    if (negative && value == 9223372036854775808ULL) {
        *result = -9223372036854775807LL - 1;
    } else if (negative) {
        *result = -(long long)value;
    } else {
        *result = (long long)value;
    }
    return 1;
}

static int parse_double(const char *text, double *result)
{
    unsigned int i = 0;
    char *end;
    double value;

    if (text[i] == '+' || text[i] == '-') {
        i++;
    }
    if (text[i] < '0' || text[i] > '9') {
        return 0;
    }
    while (text[i] >= '0' && text[i] <= '9') {
        i++;
    }
    if (text[i] != '.') {
        return 0;
    }
    i++;
    if (text[i] < '0' || text[i] > '9') {
        return 0;
    }
    while (text[i] >= '0' && text[i] <= '9') {
        i++;
    }
    if (text[i] != '\0') {
        return 0;
    }

    errno = 0;
    value = strtod(text, &end);
    if (end == text || *end != '\0') {
        return 0;
    }

    if (errno == ERANGE && (value > 1.0 || value < -1.0)) {
        return 0;
    }
    *result = value;
    return 1;
}

static int parse_number(const char *text, int *type,
                        long long *int_value, double *double_value)
{
    if (parse_int(text, int_value)) {
        *type = TYPE_INT;
        *double_value = (double)*int_value;
        return 1;
    }
    if (parse_double(text, double_value)) {
        *type = TYPE_DOUBLE;
        return 1;
    }
    return 0;
}

static int read_header(FILE *file, struct Table *table,
                       char **buffer, size_t *capacity)
{
    int separator;
    struct Column *new_columns;
    char *name;
    unsigned int i;

    do {
        separator = read_field(file, buffer, capacity);
        if (separator == -2 || (*buffer)[0] == '\0') {
            return 0;
        }
        for (i = 0; i < table->count; i++) {
            if (strcmp(table->columns[i].name, *buffer) == 0) {
                return 0;
            }
        }
        if (table->count >= 256) {
            return 0;
        }
        name = malloc(strlen(*buffer) + 1);
        if (name == NULL) {
            return 0;
        }
        strcpy(name, *buffer);
        new_columns = realloc(table->columns,
                              (table->count + 1) * sizeof *new_columns);
        if (new_columns == NULL) {
            free(name);
            return 0;
        }
        table->columns = new_columns;
        table->columns[table->count].name = name;
        table->columns[table->count].type = TYPE_INT;
        stats_reset(&table->columns[table->count]);
        table->count++;
    } while (separator == ',');

    return separator == '\n' || separator == EOF;
}

static int scan_data(FILE *file, struct Table *table,
                     char **buffer, size_t *capacity)
{
    unsigned int i;
    int separator;
    int type;
    long long int_value;
    double double_value;

    while (1) {
        for (i = 0; i < table->count; i++) {
            separator = read_field(file, buffer, capacity);
            if (separator == -2) {
                return 0;
            }

            if (i == 0 && separator == EOF && (*buffer)[0] == '\0') {
                return table->rows > 0;
            }
            if (!parse_number(*buffer, &type, &int_value, &double_value)) {
                return 0;
            }
            if (type == TYPE_DOUBLE) {
                table->columns[i].type = TYPE_DOUBLE;
            }
            if (i + 1 < table->count && separator != ',') {
                return 0;
            }
            if (i + 1 == table->count &&
                separator != '\n' && separator != EOF) {
                return 0;
            }
        }
        table->rows++;
        if (separator == EOF) {
            return 1;
        }
    }
}

static int calculate_stats(FILE *file, struct Table *table,
                           char **buffer, size_t *capacity)
{
    unsigned int i;
    unsigned long long row;
    int separator;
    int type;
    long long int_value;
    double double_value;

    rewind(file);
    if (ferror(file)) {
        return 0;
    }

    do {
        separator = read_field(file, buffer, capacity);
        if (separator == -2) {
            return 0;
        }
    } while (separator == ',');

    for (row = 0; row < table->rows; row++) {
        for (i = 0; i < table->count; i++) {
            separator = read_field(file, buffer, capacity);
            if (separator == -2 ||
                !parse_number(*buffer, &type, &int_value, &double_value)) {
                return 0;
            }
            if (table->columns[i].type == TYPE_INT) {
                stats_add_int(&table->columns[i], int_value,
                              table->rows, row == 0);
            } else {
                stats_add_double(&table->columns[i], double_value,
                                 table->rows, row == 0);
            }
        }
    }
    return !ferror(file);
}

int table_read(FILE *file, struct Table *table)
{
    char *buffer = NULL;
    size_t capacity = 0;
    int success = 0;

    if (!read_header(file, table, &buffer, &capacity)) {
        goto finish;
    }
    if (!scan_data(file, table, &buffer, &capacity)) {
        goto finish;
    }
    if (!calculate_stats(file, table, &buffer, &capacity)) {
        goto finish;
    }
    success = 1;

finish:
    free(buffer);
    return success;
}
