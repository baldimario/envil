#include "argparse.h"
#include "checks.h"
#include "types.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

/**
 * Creates an array of check options from the provided checks array.
 * Caller is responsible for freeing the returned array.
 */
struct option* get_check_options(CheckDefinition checks[], int count) {
    if (count <= 0 || !checks) {
        return NULL;
    }

    struct option* options = calloc(count + 1, sizeof(struct option));
    if (!options) {
        fprintf(stderr, "Memory allocation failed for check options\n");
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        options[i].name = checks[i].name;
        options[i].has_arg = required_argument;
        options[i].flag = NULL;
        options[i].val = 0;
    }
    // Last element is already zeroed by calloc
    return options;
}

/**
 * Creates the complete array of long options by combining base and check options.
 * Caller is responsible for freeing the returned array and the check_options.
 */
struct option* create_long_options() {
    size_t num_checks = get_check_options_count();
    size_t num_base_options = get_base_options_count();
    
    struct option* check_options = get_check_options(checks, num_checks);
    if (!check_options) {
        return NULL;
    }

    struct option* long_options = calloc(num_base_options + num_checks + 1, sizeof(struct option));
    if (!long_options) {
        free(check_options);
        fprintf(stderr, "Memory allocation failed for long options\n");
        return NULL;
    }

    memcpy(long_options, base_options, num_base_options * sizeof(struct option));
    memcpy(long_options + num_base_options, check_options, num_checks * sizeof(struct option));
    // Last element is already zeroed by calloc

    free(check_options); // We can free this now as it's been copied
    return long_options;
}

/**
 * Creates the getopt_long option string for base options.
 * Caller is responsible for freeing the returned string.
 */
char* get_getopt_long_string() {
    char* getopt_str = calloc(256, sizeof(char));
    if (!getopt_str) {
        fprintf(stderr, "Memory allocation failed for getopt string\n");
        return NULL;
    }

    int pos = 0;
    for (size_t i = 0; i < get_base_options_count() && base_options[i].name != NULL; i++) {
        if (base_options[i].val != 0) {
            getopt_str[pos++] = (char)base_options[i].val;
            if (base_options[i].has_arg == required_argument) {
                getopt_str[pos++] = ':';
            }
        }
    }

    return getopt_str;
}

/**
 * Lists all available checks with their descriptions.
 */
void list_checks() {
    printf("Available checks:\n");
    for (size_t i = 0; i < get_check_options_count(); i++) {
        printf("  --%s: %s\n", checks[i].name, checks[i].description);
    }
}

/**
 * Prints usage information and exits with status code 1.
 */
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