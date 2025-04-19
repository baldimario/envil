#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "argparse.h"
#include "types.h"
#include "validator.h"
#include "logger.h"
#include "completion.h"
#include "config.h"

static void cleanup_options(struct option* options, char* getopt_str) {
    free(options);
    free(getopt_str);
}

// Helper function to cleanup check resources
static void cleanup_checks(Check* checks, int check_count) {
    if (!checks) return;
    
    for (int i = 0; i < check_count; i++) {
        if (checks[i].definition) {
            if (strcmp(checks[i].definition->name, "enum") == 0) {
                char** values = checks[i].value.enum_values;
                if (values) {
                    for (int j = 0; values[j]; j++) {
                        free(values[j]);
                    }
                    free(values);
                }
            }
            else if (strcmp(checks[i].definition->name, "cmd") == 0) {
                free(checks[i].value.cmd_value.cmd);
            }
            else if (strcmp(checks[i].definition->name, "eq") == 0 ||
                     strcmp(checks[i].definition->name, "ne") == 0 ||
                     strcmp(checks[i].definition->name, "regex") == 0) {
                free(checks[i].value.str_value);
            }
        }
    }
    free(checks);
}

static int handle_env_option(const char* env_name, const char* default_value, bool print_value, int check_count, Check* checks) {
    ValidationErrors* errors = create_validation_errors();
    if (!errors) {
        logger(LOG_ERROR, "Failed to create validation errors structure\n");
        return 1;
    }

    char* env_value = getenv(env_name);
    int result = validate_and_print_env(env_name, env_value, default_value, print_value, checks, check_count, errors);

    if (result != ENVIL_OK && errors->count > 0) {
        for (int i = 0; i < errors->count; i++) {
            fprintf(stderr, "Error %s: %s\n", errors->errors[i].name, errors->errors[i].message);
        }
    }

    free_validation_errors(errors);
    return result;
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
    char *config_path = NULL;
    bool print_value = false;
    int verbosity = 0;  // Count of -v flags

    // Pre-allocate checks array
    Check *checks = malloc((argc / 2) * sizeof(Check)); // Maximum possible number of checks
    if (!checks) {
        logger(LOG_ERROR, "Failed to allocate memory for checks\n");
        return 1;
    }
    int check_count = 0;
    EnvType var_type = TYPE_STRING; // Default type

    struct option* long_options = create_long_options();
    if (!long_options) {
        logger(LOG_ERROR, "Failed to create options\n");
        free(checks);
        return 1;
    }

    char* getopt_str = get_getopt_long_string();
    if (!getopt_str) {
        free(long_options);
        free(checks);
        logger(LOG_ERROR, "Failed to create getopt string\n");
        return 1;
    }

    // First pass - count verbosity flags
    while ((option = getopt_long(argc, argv, getopt_str, long_options, &option_index)) != -1) {
        if (option == 'v') {
            verbosity++;
        }
    }

    // Set log level based on verbosity count
    switch (verbosity) {
        case 0:
            g_log_level = LOG_ERROR;  // Default - only errors
            break;
        case 1:
            g_log_level = LOG_INFO;   // -v - standard info
            break;
        case 2:
            g_log_level = LOG_DEBUG;  // -vv - debug info
            break;
        default:
            g_log_level = LOG_ERROR;  // -vvv or more - trace level
            break;
    }

    // Reset getopt for main option parsing
    optind = 1;  // Reset to beginning of arguments
    opterr = 1;  // Enable error messages

    // Remove debug logging that was causing issues
    while ((option = getopt_long(argc, argv, getopt_str, long_options, &option_index)) != -1) {
        switch (option) {
        case 'C': {
            // Handle shell completion generation
            ShellType shell = get_shell_type(optarg);
            if (shell == SHELL_UNKNOWN) {
                fprintf(stderr, "Error: Unsupported shell type '%s'. Supported types: bash, zsh\n", optarg);
                cleanup_options(long_options, getopt_str);
                free(checks);
                return 1;
            }
            int result = generate_completion_script(shell, stdout);
            cleanup_options(long_options, getopt_str);
            free(checks);
            return result == 0 ? 0 : 1;
        }
        case 'c':
            has_config = true;
            config_path = optarg;
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
            free(checks);
            return 0;
        case 'v':
            // Already handled in first pass
            break;
        case 'h':
        case '?':
            cleanup_options(long_options, getopt_str);
            free(checks);
            print_usage();
            break;
        case 0: // Long option without a short equivalent
            const char* check_name = long_options[option_index].name;
            const CheckDefinition* check_def = get_check_definition(check_name);
            if (check_def) {
                checks[check_count].definition = check_def;
                
                if (strcmp(check_name, "type") == 0) {
                    if (strcmp(optarg, "string") == 0) var_type = TYPE_STRING;
                    else if (strcmp(optarg, "integer") == 0) var_type = TYPE_INTEGER;
                    else if (strcmp(optarg, "float") == 0) var_type = TYPE_FLOAT;
                    else if (strcmp(optarg, "json") == 0) var_type = TYPE_JSON;
                    else {
                        fprintf(stderr, "Invalid type: %s\n", optarg);
                        cleanup_options(long_options, getopt_str);
                        free(checks);
                        return 1;
                    }
                    checks[check_count].value.int_value = var_type;
                }
                else if (strcmp(check_name, "gt") == 0 || strcmp(check_name, "lt") == 0 ||
                         strcmp(check_name, "ge") == 0 || strcmp(check_name, "le") == 0) {
                    checks[check_count].value.int_value = atoi(optarg);
                }
                else if (strcmp(check_name, "eq") == 0 || strcmp(check_name, "ne") == 0) {
                    checks[check_count].value.str_value = strdup(optarg);
                    if (!checks[check_count].value.str_value) {
                        logger(LOG_ERROR, "Failed to allocate memory for string value\n");
                        cleanup_options(long_options, getopt_str);
                        free(checks);
                        return 1;
                    }
                }
                else if (strcmp(check_name, "len") == 0 || strcmp(check_name, "lengt") == 0 ||
                         strcmp(check_name, "lenlt") == 0) {
                    checks[check_count].value.int_value = atoi(optarg);
                }
                else if (strcmp(check_name, "regex") == 0) {
                    checks[check_count].value.str_value = strdup(optarg);
                    if (!checks[check_count].value.str_value) {
                        logger(LOG_ERROR, "Failed to allocate memory for regex pattern\n");
                        cleanup_options(long_options, getopt_str);
                        free(checks);
                        return 1;
                    }
                }
                else if (strcmp(check_name, "enum") == 0) {
                    char* enum_copy = strdup(optarg);
                    char* saveptr;
                    int count = 1;
                    
                    for (const char* p = optarg; *p; p++) {
                        if (*p == ',') count++;
                    }
                    
                    char** values = malloc((count + 1) * sizeof(char*));
                    if (!values) {
                        logger(LOG_ERROR, "Failed to allocate memory for enum values\n");
                        free(enum_copy);
                        cleanup_options(long_options, getopt_str);
                        free(checks);
                        return 1;
                    }
                    
                    char* token = strtok_r(enum_copy, ",", &saveptr);
                    int i = 0;
                    while (token) {
                        values[i++] = strdup(token);
                        token = strtok_r(NULL, ",", &saveptr);
                    }
                    values[i] = NULL;
                    
                    checks[check_count].value.enum_values = values;
                    free(enum_copy);
                }
                else if (strcmp(check_name, "cmd") == 0) {
                    size_t cmd_len = strlen(optarg);
                    char* cmd_copy = malloc(cmd_len + 1);
                    if (cmd_copy) {
                        memcpy(cmd_copy, optarg, cmd_len);
                        cmd_copy[cmd_len] = '\0';
                        checks[check_count].value.cmd_value.cmd = cmd_copy;
                        checks[check_count].value.cmd_value.cmd_len = cmd_len;
                    }
                }
                
                check_count++;
            }
            break;
        default:
            break;
        }
    }

    // Validate arguments
    if (!has_config && !has_env) {
        fprintf(stderr, "Error: Must specify either -c CONFIG or -e ENV_NAME\n");
        cleanup_options(long_options, getopt_str);
        free(checks);
        return 1;
    }

    if (has_config && has_env) {
        fprintf(stderr, "Error: Cannot specify both -c and -e options\n");
        cleanup_options(long_options, getopt_str);
        free(checks);
        return 1;
    }

    // Handle single environment variable validation
    if (has_env) {
        // Verify that env_name is provided
        if (!env_name) {
            fprintf(stderr, "Error: No environment variable name provided with -e option\n");
            cleanup_options(long_options, getopt_str);
            free(checks);
            return 1;
        }

        // Set up variable with checks before calling handle_env_option
        Check *var_checks = NULL;
        int var_check_count = 0;
        if (check_count > 0) {
            var_checks = malloc(check_count * sizeof(Check));
            if (var_checks) {
                memcpy(var_checks, checks, check_count * sizeof(Check));
                var_check_count = check_count;
            }
        }
        
        int result = handle_env_option(env_name, default_value, print_value, var_check_count, var_checks);
        
        // Clean up
        if (var_checks) {
            free(var_checks);
        }
        cleanup_options(long_options, getopt_str);
        free(checks);
        return result;
    }
    // Handle config file validation
    else if (has_config) {
        int result = handle_config_option(config_path, print_value);
        cleanup_options(long_options, getopt_str);
        free(checks);
        return result;
    }

    cleanup_options(long_options, getopt_str);
    free(checks);
    return 0;
}