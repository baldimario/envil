#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <json-c/json.h>
#include "config.h"
#include "logger.h"
#include "validator.h"

struct option check_options[] = {
    {"type", required_argument, 0, 0},
    {"gt", required_argument, 0, 0},
    {"lt", required_argument, 0, 0},
    {"enum", required_argument, 0, 0},
    {"len", required_argument, 0, 0},
    {"cmd", required_argument, 0, 0},
    {"eq", required_argument, 0, 0},
    {"ne", required_argument, 0, 0},
    {"ge", required_argument, 0, 0},
    {"le", required_argument, 0, 0},
    {"lengt", required_argument, 0, 0},
    {"lenlt", required_argument, 0, 0},
    {"regex", required_argument, 0, 0},
};

CheckDefinition checks[] = {
    {"type", "Check the type of the variable (integer,string,json,float,boolean)", check_type, NULL, 1, "Invalid type"},
    {"gt", "Check if greater than a value", check_gt, NULL, 1, "Invalid length"},
    {"lt", "Check if less than a value", check_lt, NULL, 1, "Invalid length"},
    {"enum", "Check if in a set of values (foo,bar,baz)", check_enum, NULL, 1, "Invalid enum value"},
    {"len", "Check the length of the variable", check_len, NULL, 1, "Invalid length"},
    {"cmd", "Run a command to validate the variable", check_cmd, NULL, 1, "Invalid command"},
    {"eq", "Check if equal to a value", check_eq, NULL, 1, "Invalid target"},
    {"ne", "Check if not equal to a value", check_ne, NULL, 1, "Invalid target"},
    {"ge", "Check if greater than or equal to a value", check_ge, NULL, 1, "Invalid length"},
    {"le", "Check if less than or equal to a value", check_le, NULL, 1, "Invalid length"},
    {"lengt", "Check if string length is greater than specified length", check_lengt, NULL, 1, "Invalid length"},
    {"lenlt", "Check if string length is less than specified length", check_lenlt, NULL, 1, "Invalid length"},
    {"regex", "Check if value matches regular expression pattern", check_regex, NULL, 1, "Invalid pattern"},
};

struct option base_options[] = {
    {"config", required_argument, 0, 'c'},
    {"env", required_argument, 0, 'e'},
    {"default", required_argument, 0, 'd'},
    {"print", no_argument, 0, 'p'},
    {"list-checks", no_argument, 0, 'l'},
    {"verbose", no_argument, 0, 'v'},
    {"completion", required_argument, 0, 'C'},
    {"help", no_argument, 0, 'h'},
};

size_t get_base_options_count() {
    return sizeof(base_options) / sizeof(struct option);
}

size_t get_check_options_count() {
    return sizeof(check_options) / sizeof(struct option);
}

size_t get_options_count() {
    return get_base_options_count() + get_check_options_count();
}

static int process_check(const char* check_name, const char* check_value, Check* check, EnvType* var_type) {
    const CheckDefinition* check_def = get_check_definition(check_name);
    if (!check_def) {
        logger(LOG_ERROR, "Unknown check '%s'\n", check_name);
        return 0;
    }

    check->definition = check_def;

    if (strcmp(check_name, "type") == 0) {
        if (strcmp(check_value, "string") == 0) *var_type = TYPE_STRING;
        else if (strcmp(check_value, "integer") == 0) *var_type = TYPE_INTEGER;
        else if (strcmp(check_value, "float") == 0) *var_type = TYPE_FLOAT;
        else if (strcmp(check_value, "json") == 0) *var_type = TYPE_JSON;
        else {
            logger(LOG_ERROR, "Invalid type: %s\n", check_value);
            return 0;
        }
        check->value.int_value = *var_type;
    }
    else if (strcmp(check_name, "gt") == 0 || strcmp(check_name, "lt") == 0 ||
             strcmp(check_name, "ge") == 0 || strcmp(check_name, "le") == 0 ||
             strcmp(check_name, "len") == 0 || strcmp(check_name, "lengt") == 0 ||
             strcmp(check_name, "lenlt") == 0) {
        check->value.int_value = atoi(check_value);
    }
    else if (strcmp(check_name, "eq") == 0 || strcmp(check_name, "ne") == 0 ||
             strcmp(check_name, "regex") == 0) {
        check->value.str_value = strdup(check_value);
        if (!check->value.str_value) {
            logger(LOG_ERROR, "Failed to allocate memory for string value\n");
            return 0;
        }
    }
    else if (strcmp(check_name, "enum") == 0) {
        char* enum_copy = strdup(check_value);
        int count = 1;
        for (const char* p = check_value; *p; p++) {
            if (*p == ',') count++;
        }
        
        char** values = malloc((count + 1) * sizeof(char*));
        if (!values) {
            logger(LOG_ERROR, "Failed to allocate memory for enum values\n");
            free(enum_copy);
            return 0;
        }
        
        char* saveptr;
        char* token = strtok_r(enum_copy, ",", &saveptr);
        int i = 0;
        while (token) {
            values[i++] = strdup(token);
            token = strtok_r(NULL, ",", &saveptr);
        }
        values[i] = NULL;
        
        check->value.enum_values = values;
        free(enum_copy);
    }
    else if (strcmp(check_name, "cmd") == 0) {
        size_t cmd_len = strlen(check_value);
        char* cmd_copy = malloc(cmd_len + 1);
        if (!cmd_copy) {
            logger(LOG_ERROR, "Failed to allocate memory for command\n");
            return 0;
        }
        memcpy(cmd_copy, check_value, cmd_len);
        cmd_copy[cmd_len] = '\0';
        check->value.cmd_value.cmd = cmd_copy;
        check->value.cmd_value.cmd_len = cmd_len;
    }

    return 1;
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

// Helper function to validate and print environment variable
int validate_and_print_env(const char* var_name, const char* env_value, 
                          const char* default_value, bool print_value,
                          Check* checks, int check_count,
                          ValidationErrors* errors) {
    char* value = (char*)env_value;
    if (!value && default_value) {
        value = (char*)default_value;
    }

    EnvVariable var = {
        .name = (char*)var_name,
        .default_value = (char*)default_value,
        .required = (default_value == NULL),
        .type = TYPE_STRING,
        .checks = checks,
        .check_count = check_count
    };

    int result = validate_variable_with_errors(&var, value, errors);
    
    if (result == ENVIL_OK && print_value && value) {
        printf("%s=%s\n", var_name, value);
    }

    return result;
}

int handle_config_option(const char* config_path, bool print_value) {
    if (!config_path) {
        logger(LOG_ERROR, "Error: No configuration file path provided\n");
        return ENVIL_CONFIG_ERROR;
    }
    
    logger(LOG_TRACE, "Loading config file: %s", config_path);
    FILE *config_file = fopen(config_path, "r");
    if (!config_file) {
        logger(LOG_ERROR, "Error: Cannot open config file: %s\n", config_path);
        return ENVIL_CONFIG_ERROR;
    }

    // Determine file format from extension
    const char* ext = strrchr(config_path, '.');
    bool is_yaml = (ext && (strcmp(ext, ".yml") == 0 || strcmp(ext, ".yaml") == 0));
    bool is_json = (ext && strcmp(ext, ".json") == 0);

    if (!is_yaml && !is_json) {
        logger(LOG_ERROR, "Error: Config file must be .yml, .yaml, or .json\n");
        fclose(config_file);
        return ENVIL_CONFIG_ERROR;
    }

    ValidationErrors* errors = create_validation_errors();
    int result = ENVIL_OK;

    if (is_yaml) {
        result = handle_yaml_config(config_file, print_value, errors);
    } else {
        result = handle_json_config(config_file, print_value, errors);
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

int handle_yaml_config(FILE* config_file, bool print_value, ValidationErrors* errors) {
    yaml_parser_t parser;
    yaml_document_t document;
    int result = ENVIL_OK;

    if (!yaml_parser_initialize(&parser)) {
        logger(LOG_ERROR, "Failed to initialize YAML parser\n");
        return ENVIL_CONFIG_ERROR;
    }

    yaml_parser_set_input_file(&parser, config_file);

    if (!yaml_parser_load(&parser, &document)) {
        logger(LOG_ERROR, "Failed to parse YAML file\n");
        yaml_parser_delete(&parser);
        return ENVIL_CONFIG_ERROR;
    }

    yaml_node_t* root = yaml_document_get_root_node(&document);
    if (!root || root->type != YAML_MAPPING_NODE) {
        logger(LOG_ERROR, "Error: YAML root must be a mapping\n");
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        return ENVIL_CONFIG_ERROR;
    }

    // Process each variable in the YAML
    for (yaml_node_pair_t* pair = root->data.mapping.pairs.start;
         pair < root->data.mapping.pairs.top;
         pair++) {
        
        yaml_node_t* key = yaml_document_get_node(&document, pair->key);
        yaml_node_t* value_node = yaml_document_get_node(&document, pair->value);

        if (key->type != YAML_SCALAR_NODE || value_node->type != YAML_MAPPING_NODE) {
            continue;
        }

        const char* var_name = (char*)key->data.scalar.value;
        char* default_value = NULL;
        Check* checks = NULL;
        int check_count = 0;
        EnvType var_type = TYPE_STRING;

        // Process variable configuration
        for (yaml_node_pair_t* var_pair = value_node->data.mapping.pairs.start;
             var_pair < value_node->data.mapping.pairs.top;
             var_pair++) {
            
            yaml_node_t* var_key = yaml_document_get_node(&document, var_pair->key);
            yaml_node_t* var_value = yaml_document_get_node(&document, var_pair->value);
            
            if (var_key->type != YAML_SCALAR_NODE) continue;

            const char* key_name = (char*)var_key->data.scalar.value;
            if (strcmp(key_name, "default") == 0 && var_value->type == YAML_SCALAR_NODE) {
                default_value = strdup((char*)var_value->data.scalar.value);
            } else if (strcmp(key_name, "checks") == 0 && var_value->type == YAML_MAPPING_NODE) {
                int num_checks = var_value->data.mapping.pairs.top - var_value->data.mapping.pairs.start;
                if (num_checks > 0) {
                    checks = malloc(num_checks * sizeof(Check));
                    if (!checks) continue;

                    for (yaml_node_pair_t* check_pair = var_value->data.mapping.pairs.start;
                         check_pair < var_value->data.mapping.pairs.top;
                         check_pair++) {
                        
                        yaml_node_t* check_key = yaml_document_get_node(&document, check_pair->key);
                        yaml_node_t* check_value = yaml_document_get_node(&document, check_pair->value);

                        if (check_key->type != YAML_SCALAR_NODE || check_value->type != YAML_SCALAR_NODE) {
                            continue;
                        }

                        if (process_check((char*)check_key->data.scalar.value,
                                        (char*)check_value->data.scalar.value,
                                        &checks[check_count],
                                        &var_type)) {
                            check_count++;
                        }
                    }
                }
            }
        }

        // Validate the variable
        char* env_value = getenv(var_name);
        int var_result = validate_and_print_env(var_name, env_value, default_value,
                                              print_value, checks, check_count, errors);
        if (var_result != ENVIL_OK) {
            result = var_result;
        }

        free(default_value);
        cleanup_checks(checks, check_count);
    }

    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    return result;
}

int handle_json_config(FILE* config_file, bool print_value, ValidationErrors* errors) {
    struct json_object *root;
    enum json_tokener_error jerr = json_tokener_success;

    // Read entire file
    fseek(config_file, 0, SEEK_END);
    long fsize = ftell(config_file);
    fseek(config_file, 0, SEEK_SET);

    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        logger(LOG_ERROR, "Failed to allocate memory for JSON\n");
        return ENVIL_CONFIG_ERROR;
    }

    if (fread(json_str, 1, fsize, config_file) != (size_t)fsize) {
        logger(LOG_ERROR, "Failed to read JSON file\n");
        free(json_str);
        return ENVIL_CONFIG_ERROR;
    }
    json_str[fsize] = 0;

    root = json_tokener_parse_verbose(json_str, &jerr);
    free(json_str);

    if (!root || jerr != json_tokener_success) {
        logger(LOG_ERROR, "Failed to parse JSON: %s\n", json_tokener_error_desc(jerr));
        if (root) json_object_put(root);
        return ENVIL_CONFIG_ERROR;
    }

    if (json_object_get_type(root) != json_type_object) {
        logger(LOG_ERROR, "Error: JSON root must be an object\n");
        json_object_put(root);
        return ENVIL_CONFIG_ERROR;
    }

    int result = ENVIL_OK;

    // Process each variable in the JSON
    json_object_object_foreach(root, var_name, var_obj) {
        if (json_object_get_type(var_obj) != json_type_object) {
            continue;
        }

        char* default_value = NULL;
        Check* checks = NULL;
        int check_count = 0;
        EnvType var_type = TYPE_STRING;

        // Get default value if present
        struct json_object* default_obj;
        if (json_object_object_get_ex(var_obj, "default", &default_obj)) {
            default_value = strdup(json_object_get_string(default_obj));
        }

        // Process checks if present
        struct json_object* checks_obj;
        if (json_object_object_get_ex(var_obj, "checks", &checks_obj) &&
            json_object_get_type(checks_obj) == json_type_object) {
            
            int num_checks = json_object_object_length(checks_obj);
            if (num_checks > 0) {
                checks = malloc(num_checks * sizeof(Check));
                if (checks) {
                    json_object_object_foreach(checks_obj, check_name, check_value) {
                        if (process_check(check_name,
                                        json_object_get_string(check_value),
                                        &checks[check_count],
                                        &var_type)) {
                            check_count++;
                        }
                    }
                }
            }
        }

        // Validate the variable
        char* env_value = getenv(var_name);
        int var_result = validate_and_print_env(var_name, env_value, default_value,
                                              print_value, checks, check_count, errors);
        if (var_result != ENVIL_OK) {
            result = var_result;
        }

        free(default_value);
        cleanup_checks(checks, check_count);
    }

    json_object_put(root);
    return result;
}