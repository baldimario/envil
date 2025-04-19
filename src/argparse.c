#include "argparse.h"
#include "checks.h"
#include "types.h"
#include "config.h"
#include "logger.h"
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
        logger(LOG_ERROR, "Memory allocation failed for check options\n");
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
    // Get the number of base options and check options
    size_t num_base = get_base_options_count();
    size_t num_checks = get_check_options_count();
    
    // Allocate space for the combined array (+1 for NULL terminator)
    struct option* long_options = calloc(num_base + num_checks + 1, sizeof(struct option));
    if (!long_options) {
        logger(LOG_ERROR, "Memory allocation failed for long options\n");
        return NULL;
    }

    // Copy base options
    memcpy(long_options, base_options, num_base * sizeof(struct option));
    
    // Add check options
    for (size_t i = 0; i < num_checks; i++) {
        struct option* opt = &long_options[num_base + i];
        opt->name = checks[i].name;
        opt->has_arg = required_argument;
        opt->flag = NULL;
        opt->val = 0;  // Use 0 for check options to distinguish from base options
    }

    // Last element is already zeroed by calloc
    return long_options;
}

/**
 * Creates the getopt_long option string for base options.
 * Caller is responsible for freeing the returned string.
 */
char* get_getopt_long_string() {
    char* getopt_str = calloc(256, sizeof(char));
    if (!getopt_str) {
        logger(LOG_ERROR, "Memory allocation failed for getopt string\n");
        return NULL;
    }

    const char* valid_options = "c:e:pvlhC:d:"; // Colon after options that require arguments

    // Copy valid options to getopt string
    strcpy(getopt_str, valid_options);

    return getopt_str;
}

/**
 * Lists all available checks with their descriptions.
 */
void list_checks() {
    fprintf(stderr, "Available checks:\n");
    for (size_t i = 0; i < get_check_options_count(); i++) {
        fprintf(stderr, "  --%s: %s\n", checks[i].name, checks[i].description);
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
    fprintf(stderr, "  Generate completion: envil -C <shell>\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -c, --config FILE    Path to configuration file (YAML or JSON)\n");
    fprintf(stderr, "  -e, --env NAME       Environment variable name\n");
    fprintf(stderr, "  -d, --default VALUE  Default value if not set\n");
    fprintf(stderr, "  -p, --print          Print value if validation passes\n");
    fprintf(stderr, "  -v, --verbose        Enable verbose output\n");
    fprintf(stderr, "  -l, --list-checks    List available check types and descriptions\n");
    fprintf(stderr, "  -C, --completion <shell>  Generate shell completion script (bash|zsh)\n");
    fprintf(stderr, "  -h, --help           Show this help message\n");
    exit(1);
}