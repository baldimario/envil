#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <yaml.h>
#include <json-c/json.h>
#include "argparse.h"
#include "types.h"
#include "validator.h"

LogLevel g_log_level = LOG_INFO;  // Define the global log level with default value

static void cleanup_options(struct option* options, char* getopt_str) {
    free(options);
    free(getopt_str);
}

static int handle_env_option(const char* env_name, const char* default_value, bool print_value, int check_count, Check* checks, int first_check_arg) {
    // Create variable structure with proper const qualifiers
    EnvVariable var;
    var.name = (char*)env_name;  // Cast away const since the struct doesn't expect const
    var.default_value = (char*)default_value;
    var.required = (default_value == NULL);
    var.type = TYPE_STRING;
    var.checks = checks;
    var.check_count = check_count;

    // Get environment variable value
    char* value = getenv(var.name);
    if (!value && var.default_value) {
        value = var.default_value;
    }

    // Create validation errors structure
    ValidationErrors* errors = create_validation_errors();
    if (!errors) {
        fprintf(stderr, "Failed to create validation errors structure\n");
        return 1;
    }

    // Validate the variable
    int result = validate_variable_with_errors(&var, value, errors);
    
    // Print any validation errors
    if (result != ENVIL_OK) {
        for (int i = 0; i < errors->count; i++) {
            fprintf(stderr, "Error %s: %s\n", errors->errors[i].name, errors->errors[i].message);
        }
    }
    
    // Print value if requested and validation passed
    if (print_value && result == ENVIL_OK && value) {
        printf("%s\n", value);
    }

    // Only free the validation errors - checks are owned by caller
    free_validation_errors(errors);
    return result;
}

static int handle_config_option(const char* config_path, bool print_value) {
    if (!config_path) {
        fprintf(stderr, "Error: No configuration file path provided\n");
        return ENVIL_CONFIG_ERROR;
    }
    
    fprintf(stderr, "Loading config file: %s\n", config_path);
    FILE *config_file = fopen(config_path, "r");
    if (!config_file) {
        fprintf(stderr, "Error: Cannot open config file: %s\n", config_path);
        return ENVIL_CONFIG_ERROR;
    }

    // Determine file format from extension
    const char* ext = strrchr(config_path, '.');
    bool is_yaml = (ext && (strcmp(ext, ".yml") == 0 || strcmp(ext, ".yaml") == 0));
    bool is_json = (ext && strcmp(ext, ".json") == 0);

    if (!is_yaml && !is_json) {
        fprintf(stderr, "Error: Config file must be .yml, .yaml, or .json\n");
        fclose(config_file);
        return ENVIL_CONFIG_ERROR;
    }

    ValidationErrors* errors = create_validation_errors();
    int result = ENVIL_OK;

    if (is_yaml) {
        yaml_parser_t parser;
        yaml_document_t document;

        if (!yaml_parser_initialize(&parser)) {
            fprintf(stderr, "Failed to initialize YAML parser\n");
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }

        yaml_parser_set_input_file(&parser, config_file);

        if (!yaml_parser_load(&parser, &document)) {
            fprintf(stderr, "Failed to parse YAML file\n");
            yaml_parser_delete(&parser);
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }

        yaml_node_t* root = yaml_document_get_root_node(&document);
        if (!root || root->type != YAML_MAPPING_NODE) {
            fprintf(stderr, "Error: YAML root must be a mapping\n");
            yaml_document_delete(&document);
            yaml_parser_delete(&parser);
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }

        // Iterate through each variable in the YAML
        for (yaml_node_pair_t* pair = root->data.mapping.pairs.start;
             pair < root->data.mapping.pairs.top;
             pair++) {
            
            yaml_node_t* key = yaml_document_get_node(&document, pair->key);
            yaml_node_t* value_node = yaml_document_get_node(&document, pair->value);

            if (key->type != YAML_SCALAR_NODE || value_node->type != YAML_MAPPING_NODE) {
                continue;
            }

            const char* var_name = (char*)key->data.scalar.value;
            EnvVariable var;
            var.name = (char*)var_name;
            var.type = TYPE_STRING;
            var.required = true;
            var.default_value = NULL;
            var.checks = NULL;
            var.check_count = 0;

            // Count checks to allocate memory
            int num_checks = 0;
            yaml_node_t* checks_node = NULL;
            for (yaml_node_pair_t* var_pair = value_node->data.mapping.pairs.start;
                 var_pair < value_node->data.mapping.pairs.top;
                 var_pair++) {
                
                yaml_node_t* var_key = yaml_document_get_node(&document, var_pair->key);
                if (strcmp((char*)var_key->data.scalar.value, "checks") == 0) {
                    checks_node = yaml_document_get_node(&document, var_pair->value);
                    if (checks_node->type == YAML_MAPPING_NODE) {
                        num_checks = checks_node->data.mapping.pairs.top - checks_node->data.mapping.pairs.start;
                    }
                } else if (strcmp((char*)var_key->data.scalar.value, "default") == 0) {
                    yaml_node_t* default_node = yaml_document_get_node(&document, var_pair->value);
                    if (default_node->type == YAML_SCALAR_NODE) {
                        var.default_value = strdup((char*)default_node->data.scalar.value);
                        var.required = false;
                    }
                }
            }

            // Allocate and populate checks
            if (num_checks > 0 && checks_node) {
                Check* checks = malloc(num_checks * sizeof(Check));
                if (!checks) {
                    fprintf(stderr, "Failed to allocate memory for checks\n");
                    continue;
                }
                var.checks = checks;

                // Process checks
                int check_index = 0;
                for (yaml_node_pair_t* check_pair = checks_node->data.mapping.pairs.start;
                     check_pair < checks_node->data.mapping.pairs.top;
                     check_pair++) {
                    
                    yaml_node_t* check_key = yaml_document_get_node(&document, check_pair->key);
                    yaml_node_t* check_value = yaml_document_get_node(&document, check_pair->value);

                    if (check_key->type != YAML_SCALAR_NODE || check_value->type != YAML_SCALAR_NODE) {
                        continue;
                    }

                    const char* check_name = (char*)check_key->data.scalar.value;
                    const char* check_val = (char*)check_value->data.scalar.value;
                    const CheckDefinition* check_def = get_check_definition(check_name);

                    if (!check_def) {
                        fprintf(stderr, "Unknown check '%s' for variable '%s'\n", check_name, var_name);
                        continue;
                    }

                    checks[check_index].definition = check_def;

                    // Handle different check types
                    if (strcmp(check_name, "type") == 0) {
                        if (strcmp(check_val, "string") == 0) var.type = TYPE_STRING;
                        else if (strcmp(check_val, "integer") == 0) var.type = TYPE_INTEGER;
                        else if (strcmp(check_val, "float") == 0) var.type = TYPE_FLOAT;
                        else if (strcmp(check_val, "json") == 0) var.type = TYPE_JSON;
                        checks[check_index].value.int_value = var.type;
                    }
                    else if (strcmp(check_name, "gt") == 0 || strcmp(check_name, "lt") == 0) {
                        checks[check_index].value.int_value = atoi(check_val);
                    }
                    else if (strcmp(check_name, "len") == 0) {
                        checks[check_index].value.int_value = atoi(check_val);
                    }
                    else if (strcmp(check_name, "enum") == 0) {
                        char* enum_copy = strdup(check_val);
                        int count = 1;
                        for (const char* p = check_val; *p; p++) {
                            if (*p == ',') count++;
                        }
                        
                        char** values = malloc((count + 1) * sizeof(char*));
                        if (values) {
                            char* saveptr;
                            char* token = strtok_r(enum_copy, ",", &saveptr);
                            int i = 0;
                            while (token) {
                                values[i++] = strdup(token);
                                token = strtok_r(NULL, ",", &saveptr);
                            }
                            values[i] = NULL;
                            checks[check_index].value.enum_values = values;
                        }
                        free(enum_copy);
                    }
                    else if (strcmp(check_name, "cmd") == 0) {
                        const char* cmd_str = (char*)check_value->data.scalar.value;
                        size_t cmd_len = check_value->data.scalar.length;
                        if (cmd_len > 0) {
                            char* cmd_copy = malloc(cmd_len + 1);
                            if (cmd_copy) {
                                memcpy(cmd_copy, cmd_str, cmd_len);
                                cmd_copy[cmd_len] = '\0';
                                checks[check_index].value.cmd_value.cmd = cmd_copy;
                                checks[check_index].value.cmd_value.cmd_len = cmd_len;
                            }
                        } else {
                            fprintf(stderr, "Warning: Empty command for variable '%s'\n", var_name);
                            continue;
                        }
                    }

                    check_index++;
                }
                var.check_count = check_index;
            }

            // Get and validate variable value
            char* env_value = getenv(var.name);
            if (!env_value && var.default_value) {
                env_value = var.default_value;
            }

            int var_result = validate_variable_with_errors(&var, env_value, errors);
            if (var_result != ENVIL_OK) {
                result = var_result;
            } else if (print_value && env_value) {
                printf("%s=%s\n", var.name, env_value);
            }

            // Cleanup variable resources
            if (var.default_value) {
                free((void*)var.default_value);
            }
            if (var.checks) {
                for (int i = 0; i < var.check_count; i++) {
                    if (var.checks[i].definition) {
                        if (strcmp(var.checks[i].definition->name, "enum") == 0) {
                            char** values = var.checks[i].value.enum_values;
                            if (values) {
                                for (int j = 0; values[j]; j++) {
                                    free(values[j]);
                                }
                                free(values);
                            }
                        }
                        else if (strcmp(var.checks[i].definition->name, "cmd") == 0) {
                            free(var.checks[i].value.cmd_value.cmd);
                        }
                    }
                }
                free(var.checks);
            }
        }

        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
    }
    else if (is_json) {
        struct json_object *root;
        enum json_tokener_error jerr = json_tokener_success;

        // Read entire file
        fseek(config_file, 0, SEEK_END);
        long fsize = ftell(config_file);
        fseek(config_file, 0, SEEK_SET);

        char *json_str = malloc(fsize + 1);
        if (!json_str) {
            fprintf(stderr, "Failed to allocate memory for JSON\n");
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }

        if (fread(json_str, 1, fsize, config_file) != (size_t)fsize) {
            fprintf(stderr, "Failed to read JSON file\n");
            free(json_str);
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }
        json_str[fsize] = 0;

        root = json_tokener_parse_verbose(json_str, &jerr);
        free(json_str);

        if (!root || jerr != json_tokener_success) {
            fprintf(stderr, "Failed to parse JSON: %s\n", json_tokener_error_desc(jerr));
            if (root) json_object_put(root);
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }

        if (json_object_get_type(root) != json_type_object) {
            fprintf(stderr, "Error: JSON root must be an object\n");
            json_object_put(root);
            fclose(config_file);
            free_validation_errors(errors);
            return ENVIL_CONFIG_ERROR;
        }

        // Iterate through each variable in the JSON
        json_object_object_foreach(root, var_name, var_obj) {
            if (json_object_get_type(var_obj) != json_type_object) {
                continue;
            }

            EnvVariable var;
            var.name = (char*)var_name;
            var.type = TYPE_STRING;
            var.required = true;
            var.default_value = NULL;
            var.checks = NULL;
            var.check_count = 0;

            // Get default value if present
            struct json_object* default_obj;
            if (json_object_object_get_ex(var_obj, "default", &default_obj)) {
                var.default_value = strdup(json_object_get_string(default_obj));
                var.required = false;
            }

            // Get checks if present
            struct json_object* checks_obj;
            if (json_object_object_get_ex(var_obj, "checks", &checks_obj) &&
                json_object_get_type(checks_obj) == json_type_object) {
                
                int num_checks = json_object_object_length(checks_obj);
                if (num_checks > 0) {
                    Check* checks = malloc(num_checks * sizeof(Check));
                    if (!checks) {
                        fprintf(stderr, "Failed to allocate memory for checks\n");
                        continue;
                    }
                    var.checks = checks;

                    // Process checks
                    int check_index = 0;
                    json_object_object_foreach(checks_obj, check_name, check_value) {
                        const CheckDefinition* check_def = get_check_definition(check_name);
                        if (!check_def) {
                            fprintf(stderr, "Unknown check '%s' for variable '%s'\n", check_name, var_name);
                            continue;
                        }

                        checks[check_index].definition = check_def;
                        const char* check_val = json_object_get_string(check_value);

                        // Handle different check types
                        if (strcmp(check_name, "type") == 0) {
                            if (strcmp(check_val, "string") == 0) var.type = TYPE_STRING;
                            else if (strcmp(check_val, "integer") == 0) var.type = TYPE_INTEGER;
                            else if (strcmp(check_val, "float") == 0) var.type = TYPE_FLOAT;
                            else if (strcmp(check_val, "json") == 0) var.type = TYPE_JSON;
                            checks[check_index].value.int_value = var.type;
                        }
                        else if (strcmp(check_name, "gt") == 0 || strcmp(check_name, "lt") == 0) {
                            checks[check_index].value.int_value = atoi(check_val);
                        }
                        else if (strcmp(check_name, "len") == 0) {
                            checks[check_index].value.int_value = atoi(check_val);
                        }
                        else if (strcmp(check_name, "enum") == 0) {
                            char* enum_copy = strdup(check_val);
                            int count = 1;
                            for (const char* p = check_val; *p; p++) {
                                if (*p == ',') count++;
                            }
                            
                            char** values = malloc((count + 1) * sizeof(char*));
                            if (values) {
                                char* saveptr;
                                char* token = strtok_r(enum_copy, ",", &saveptr);
                                int i = 0;
                                while (token) {
                                    values[i++] = strdup(token);
                                    token = strtok_r(NULL, ",", &saveptr);
                                }
                                values[i] = NULL;
                                checks[check_index].value.enum_values = values;
                            }
                            free(enum_copy);
                        }
                        else if (strcmp(check_name, "cmd") == 0) {
                            size_t cmd_len = json_object_get_string_len(check_value);
                            const char* cmd_str = json_object_get_string(check_value);
                            if (cmd_len > 0) {
                                char* cmd_copy = malloc(cmd_len + 1);
                                if (cmd_copy) {
                                    memcpy(cmd_copy, cmd_str, cmd_len);
                                    cmd_copy[cmd_len] = '\0';
                                    checks[check_index].value.cmd_value.cmd = cmd_copy;
                                    checks[check_index].value.cmd_value.cmd_len = cmd_len;
                                }
                            } else {
                                fprintf(stderr, "Warning: Empty command for variable '%s'\n", var_name);
                                continue;
                            }
                        }

                        check_index++;
                    }
                    var.check_count = check_index;
                }
            }

            // Get and validate variable value
            char* env_value = getenv(var.name);
            if (!env_value && var.default_value) {
                env_value = var.default_value;
            }

            int var_result = validate_variable_with_errors(&var, env_value, errors);
            if (var_result != ENVIL_OK) {
                result = var_result;
            } else if (print_value && env_value) {
                printf("%s=%s\n", var.name, env_value);
            }

            // Cleanup variable resources
            if (var.default_value) {
                free((void*)var.default_value);
            }
            if (var.checks) {
                for (int i = 0; i < var.check_count; i++) {
                    if (var.checks[i].definition) {
                        if (strcmp(var.checks[i].definition->name, "enum") == 0) {
                            char** values = var.checks[i].value.enum_values;
                            if (values) {
                                for (int j = 0; values[j]; j++) {
                                    free(values[j]);
                                }
                                free(values);
                            }
                        }
                        else if (strcmp(var.checks[i].definition->name, "cmd") == 0) {
                            free(var.checks[i].value.cmd_value.cmd);
                        }
                    }
                }
                free(var.checks);
            }
        }

        json_object_put(root);
    }

    // Print any validation errors
    if (result != ENVIL_OK && errors->count > 0) {
        for (int i = 0; i < errors->count; i++) {
            fprintf(stderr, "Error %s: %s\n", errors->errors[i].name, errors->errors[i].message);
        }
    }

    fclose(config_file);
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
            config_path = optarg;  // Store the config file path
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
        
        int result = handle_env_option(env_name, default_value, print_value, var_check_count, var_checks, optind);
        
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