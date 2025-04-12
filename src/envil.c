#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "argparse.h"
#include "types.h"

LogLevel g_log_level = LOG_INFO;  // Define the global log level with default value

static void cleanup_options(struct option* options, char* getopt_str) {
    free(options);
    free(getopt_str);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    int option, option_index = 0;
    bool has_config = false;
    bool has_env = false;
    char *env_name = NULL;
    char *default_value = NULL;
    bool print_value = false;

    struct option* long_options = create_long_options();
    if (!long_options) {
        fprintf(stderr, "Failed to create options\n");
        return 1;
    }

    char* getopt_str = get_getopt_long_string();
    if (!getopt_str) {
        free(long_options);
        fprintf(stderr, "Failed to create getopt string\n");
        return 1;
    }

    while ((option = getopt_long(argc, argv, getopt_str, long_options, &option_index)) != -1) {
        switch (option) {
        case 'c':
            has_config = true;
            // Config file path is in optarg
            break;
        case 'e':
            has_env = true;
            env_name = optarg;
            break;
        case 'd':
            default_value = optarg;
            break;
        case 'p':
            print_value = true;
            break;
        case 'l':
            list_checks();
            cleanup_options(long_options, getopt_str);
            return 0;
        case 'v':
            g_log_level = LOG_DEBUG;
            break;
        case 'h':
        case '?':
            cleanup_options(long_options, getopt_str);
            print_usage();
            break;
        default:
            break;
        }
    }

    // Validate arguments
    if (!has_config && !has_env) {
        fprintf(stderr, "Error: Must specify either -c CONFIG or -e ENV_NAME\n");
        cleanup_options(long_options, getopt_str);
        return 1;
    }

    if (has_config && has_env) {
        fprintf(stderr, "Error: Cannot specify both -c and -e options\n");
        cleanup_options(long_options, getopt_str);
        return 1;
    }

    cleanup_options(long_options, getopt_str);
    return 0;
}