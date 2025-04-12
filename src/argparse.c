#include "argparse.h"
#include "checks.h"
#include "types.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

struct option* get_check_options(CheckDefinition checks[], int count) {
    struct option* options = malloc((count + 1) * sizeof(struct option));
    if (!options) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(255);
    }
    for (int i = 0; i < count; i++) {
        options[i].name = checks[i].name;
        options[i].has_arg = required_argument;
        options[i].flag = NULL;
        options[i].val = 0;
    }
    return options;
}
struct option* create_long_options()  {
    size_t num_checks = get_check_options_count();
    size_t num_base_options = get_base_options_count();
    struct option* check_options = get_check_options(checks, num_checks);

    struct option* long_options = calloc(num_base_options + num_checks + 1, sizeof(struct option));
    if (!long_options) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(255);
    }

    memcpy(long_options, base_options, get_base_options_count() * sizeof(struct option));
    memcpy(long_options + num_base_options, check_options, num_checks * sizeof(struct option));

    long_options[num_base_options + num_checks] = (struct option){0, 0, 0, 0};

    return long_options;
}

char* get_getopt_long_string() {
    char* getopt_str = malloc(256);
    if (!getopt_str) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(255);
    }
    memset(getopt_str, 0, 256);

    int pos = 0;
    for (int i = 0; base_options[i].name != NULL; i++) {
        if (base_options[i].val != 0) {
            getopt_str[pos++] = (char)base_options[i].val;
            if (base_options[i].has_arg == required_argument) {
                getopt_str[pos++] = ':';
            }
        }
    }
    return getopt_str;
}

void list_checks() {
    printf("Available checks:\n");
    for (unsigned long int i = 0; i < get_check_options_count(); i++) {
        printf("  --%s: %s\n", checks[i].name, checks[i].description);
    }
}

void print_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  Single variable: envil -e VAR_NAME [-d VALUE] [-p]\n");
    fprintf(stderr, "  Config file: envil -c config.yml\n");
    fprintf(stderr, "  List checks: envil -l\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -c, --config FILE    Path to configuration file (YAML or JSON)\n");
    fprintf(stderr, "  -e, --env NAME       Environment variable name\n");
    fprintf(stderr, "  -d, --default VALUE  Default value if not set\n");
    fprintf(stderr, "  -p, --print          Print value if validation passes\n");
    fprintf(stderr, "  -v, --verbose        Enable verbose output\n");
    fprintf(stderr, "  -l, --list-checks    List available check types and descriptions\n");
    fprintf(stderr, "  -h, --help           Show this help message\n");
    exit(1);
}