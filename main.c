#include "table.h"
#include "stats.h"
#include <stdio.h>
#include <string.h>

static void print_help(void)
{
    printf("Usage:\n");
    printf("  csvstat --help\n");
    printf("  csvstat schema <csv-file>\n");
    printf("  csvstat stats <csv-file>\n");
}

int main(int argc, char *argv[])
{
    FILE *file;
    struct Table table;
    int schema_mode;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }
    if (argc != 3 ||
        (strcmp(argv[1], "schema") != 0 &&
         strcmp(argv[1], "stats") != 0)) {
        fprintf(stderr, "Invalid command line arguments\n");
        return 2;
    }

    schema_mode = strcmp(argv[1], "schema") == 0;
    file = fopen(argv[2], "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open input file\n");
        return 1;
    }

    table_init(&table);
    if (!table_read(file, &table)) {
        fprintf(stderr, "Cannot read CSV table\n");
        table_free(&table);
        fclose(file);
        return 1;
    }
    if (fclose(file) != 0) {
        fprintf(stderr, "Cannot close input file\n");
        table_free(&table);
        return 1;
    }

    if (schema_mode) {
        print_schema(&table);
    } else {
        print_stats(&table);
    }
    table_free(&table);
    return 0;
}
