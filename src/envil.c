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

LogLevel g_log_level = LOG_INFO;  // Define the global log level with default value

static void cleanup_options(struct option* options, char* getopt_str) {
    free(options);
    free(getopt_str);
}

static int handle_env_option(const char* env_name, const char* default_value, bool print_value, int argc, char** argv, int first_check_arg) {
    // Create variable structure
    EnvVariable var = {
        .name = env_name,
        .default_value = default_value,
        .required = (default_value == NULL),
        .type = TYPE_STRING, // Default type
        .checks = NULL,
        .check_count = 0
    };

    // Reset getopt state
    optind = first_check_arg;
    
    // Create long options specifically for checks
    struct option* check_options = create_long_options();
    if (!check_options) {
        fprintf(stderr, "Failed to create check options\n");
        return 1;
    }

    // Allocate space for up to (argc - first_check_arg)/2 checks
    int max_checks = (argc - first_check_arg) / 2;
    Check *checks = malloc(max_checks * sizeof(Check));
    if (!checks) {
        fprintf(stderr, "Failed to allocate memory for checks\n");
        free(check_options);
        return 1;
    }
    var.checks = checks;

    // Process remaining arguments as long options
    int option;
    int option_index = 0;
    while ((option = getopt_long(argc, argv, "+", check_options, &option_index)) != -1) {
        const char* check_name = check_options[option_index].name;
        const char* check_value = optarg;

        // Look up check definition
        const CheckDefinition* check_def = get_check_definition(check_name);
        if (!check_def) {
            fprintf(stderr, "Unknown check: %s\n", check_name);
            free(checks);
            free(check_options);
            return 1;
        }

        // Add check to variable
        checks[var.check_count].definition = check_def;
        
        // Handle different check types
        if (strcmp(check_name, "type") == 0) {
            if (strcmp(check_value, "string") == 0) var.type = TYPE_STRING;
            else if (strcmp(check_value, "integer") == 0) var.type = TYPE_INTEGER;
            else if (strcmp(check_value, "float") == 0) var.type = TYPE_FLOAT;
            else if (strcmp(check_value, "json") == 0) var.type = TYPE_JSON;
            else {
                fprintf(stderr, "Invalid type: %s\n", check_value);
                free(checks);
                free(check_options);
                return 1;
            }
            checks[var.check_count].value.int_value = var.type;
        }
        else if (strcmp(check_name, "gt") == 0 || strcmp(check_name, "lt") == 0) {
            checks[var.check_count].value.int_value = atoi(check_value);
        }
        else if (strcmp(check_name, "len") == 0) {
            checks[var.check_count].value.int_value = atoi(check_value);
        }
        else if (strcmp(check_name, "enum") == 0) {
            char* enum_copy = strdup(check_value);
            char* saveptr;
            int count = 1;
            
            // Count commas to determine array size
            for (const char* p = check_value; *p; p++) {
                if (*p == ',') count++;
            }
            
            // Allocate array of string pointers (+1 for NULL terminator)
            char** values = malloc((count + 1) * sizeof(char*));
            if (!values) {
                fprintf(stderr, "Failed to allocate memory for enum values\n");
                free(enum_copy);
                free(checks);
                free(check_options);
                return 1;
            }
            
            // Split string into array
            char* token = strtok_r(enum_copy, ",", &saveptr);
            int i = 0;
            while (token) {
                values[i++] = strdup(token);
                token = strtok_r(NULL, ",", &saveptr);
            }
            values[i] = NULL;
            
            checks[var.check_count].value.enum_values = values;
            free(enum_copy);
        }
        else if (strcmp(check_name, "cmd") == 0) {
            checks[var.check_count].value.cmd = strdup(check_value);
        }
        
        var.check_count++;
    }

    // Get environment variable value
    char* value = getenv(var.name);
    if (!value && var.default_value) {
        value = var.default_value;
    }

    // Create validation errors structure
    ValidationErrors* errors = create_validation_errors();
    if (!errors) {
        fprintf(stderr, "Failed to create validation errors structure\n");
        free(checks);
        free(check_options);
        return 1;
    }

    // Validate the variable
    int result = validate_variable_with_errors(&var, value, errors);
    
    // Print any validation errors
    if (result != ENVIL_OK) {
        for (int i = 0; i < errors->count; i++) {
            fprintf(stderr, "Error: %s\n", errors->errors[i].message);
        }
    }
    
    // Print value if requested and validation passed
    if (print_value && result == ENVIL_OK && value) {
        printf("%s\n", value);
    }

    // Cleanup
    for (int i = 0; i < var.check_count; i++) {
        if (var.checks[i].definition && strcmp(var.checks[i].definition->name, "enum") == 0) {
            char** values = var.checks[i].value.enum_values;
            if (values) {
                for (int j = 0; values[j]; j++) {
                    free(values[j]);
                }
                free(values);
            }
        }
        else if (var.checks[i].definition && strcmp(var.checks[i].definition->name, "cmd") == 0) {
            free(var.checks[i].value.cmd);
        }
    }
    free(checks);
    free(check_options);
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
    bool print_value = false;

    // Pre-allocate checks array
    Check *checks = malloc((argc / 2) * sizeof(Check)); // Maximum possible number of checks
    if (!checks) {
        fprintf(stderr, "Failed to allocate memory for checks\n");
        return 1;
    }
    int check_count = 0;
    EnvType var_type = TYPE_STRING; // Default type

    struct option* long_options = create_long_options();
    if (!long_options) {
        fprintf(stderr, "Failed to create options\n");
        free(checks);
        return 1;
    }

    char* getopt_str = get_getopt_long_string();
    if (!getopt_str) {
        free(long_options);
        free(checks);
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
            free(checks);
            return 0;
        case 'v':
            g_log_level = LOG_DEBUG;
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
                else if (strcmp(check_name, "gt") == 0 || strcmp(check_name, "lt") == 0) {
                    checks[check_count].value.int_value = atoi(optarg);
                }
                else if (strcmp(check_name, "len") == 0) {
                    checks[check_count].value.int_value = atoi(optarg);
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
                        fprintf(stderr, "Failed to allocate memory for enum values\n");
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
                    checks[check_count].value.cmd = strdup(optarg);
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
        // Create variable structure
        EnvVariable var = {
            .name = env_name,
            .default_value = default_value,
            .required = (default_value == NULL),
            .type = var_type,
            .checks = checks,
            .check_count = check_count
        };

        // Get environment variable value
        char* value = getenv(var.name);
        if (!value && var.default_value) {
            value = var.default_value;
        }

        // Create validation errors structure
        ValidationErrors* errors = create_validation_errors();
        if (!errors) {
            fprintf(stderr, "Failed to create validation errors structure\n");
            cleanup_options(long_options, getopt_str);
            free(checks);
            return 1;
        }

        // Validate the variable
        int result = validate_variable_with_errors(&var, value, errors);
        
        // Print any validation errors
        if (result != ENVIL_OK) {
            for (int i = 0; i < errors->count; i++) {
                fprintf(stderr, "Error: %s\n", errors->errors[i].message);
            }
        }
        
        // Print value if requested and validation passed
        if (print_value && result == ENVIL_OK && value) {
            printf("%s\n", value);
        }

        // Cleanup
        for (int i = 0; i < check_count; i++) {
            if (checks[i].definition && strcmp(checks[i].definition->name, "enum") == 0) {
                char** values = checks[i].value.enum_values;
                if (values) {
                    for (int j = 0; values[j]; j++) {
                        free(values[j]);
                    }
                    free(values);
                }
            }
            else if (checks[i].definition && strcmp(checks[i].definition->name, "cmd") == 0) {
                free(checks[i].value.cmd);
            }
        }
        free(checks);
        free_validation_errors(errors);
        cleanup_options(long_options, getopt_str);
        return result;
    }

    cleanup_options(long_options, getopt_str);
    free(checks);
    return 0;
}