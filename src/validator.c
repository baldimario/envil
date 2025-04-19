#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <json-c/json.h>
#include "validator.h"
#include "types.h"
#include "logger.h"

#define INITIAL_ERROR_CAPACITY 8

bool is_integer(const char *str) {
    if (!str || !*str) return false;
    
    // Handle negative numbers
    if (*str == '-') str++;
    
    // Check that all remaining characters are digits
    while (*str) {
        if (!isdigit(*str)) return false;
        str++;
    }
    return true;
}

bool is_float(const char *str) {
    if (!str || !*str) return false;
    
    char *endptr;
    strtod(str, &endptr);
    return *endptr == '\0';
}

bool is_json(const char *str) {
    if (!str || !*str) return false;
    
    struct json_object *obj = json_tokener_parse(str);
    if (obj == NULL) {
        return false;
    }
    
    json_object_put(obj); // Free the object
    return true;
}

int validate_type(EnvType type, const char *value) {
    if (!value) return ENVIL_TYPE_ERROR;

    switch (type) {
        case TYPE_STRING:
            return ENVIL_OK;
        case TYPE_INTEGER:
            return is_integer(value) ? ENVIL_OK : ENVIL_TYPE_ERROR;
        case TYPE_JSON:
            return is_json(value) ? ENVIL_OK : ENVIL_TYPE_ERROR;
        case TYPE_FLOAT:
            return is_float(value) ? ENVIL_OK : ENVIL_TYPE_ERROR;
        default:
            return ENVIL_TYPE_ERROR;
    }
}

int validate_check(const Check *check, const char *value) {
    if (!check || !value || !check->definition) return ENVIL_VALUE_ERROR;
    
    if (strcmp(check->definition->name, "enum") == 0) {
        return check->definition->callback(value, check->value.enum_values);
    }
    if (strcmp(check->definition->name, "cmd") == 0) {
        return check->definition->callback(value, &check->value.cmd_value);
    }
    if (strcmp(check->definition->name, "type") == 0) {
        return check->definition->callback(value, &check->value.int_value);
    }
    if (strcmp(check->definition->name, "len") == 0) {
        return check->definition->callback(value, &check->value.int_value);
    }
    if (strcmp(check->definition->name, "gt") == 0 || strcmp(check->definition->name, "lt") == 0) {
        return check->definition->callback(value, &check->value.int_value);
    }
    if (strcmp(check->definition->name, "eq") == 0 || strcmp(check->definition->name, "ne") == 0) {
        return check->definition->callback(value, check->value.str_value);
    }
    if (strcmp(check->definition->name, "lengt") == 0 || strcmp(check->definition->name, "lenlt") == 0) {
        return check->definition->callback(value, &check->value.str_value);
    }
    if (strcmp(check->definition->name, "regex") == 0) {
        return check->definition->callback(value, check->value.str_value);
    }
    if (strcmp(check->definition->name, "ge") == 0 || strcmp(check->definition->name, "le") == 0) {
        return check->definition->callback(value, &check->value.int_value);
    }
    if (strcmp(check->definition->name, "regex") == 0) {
        return check->definition->callback(value, &check->value.str_value);
    }
    return check->definition->callback(value, &check->value);
}

int validate_variable(const EnvVariable *var, const char *value) {
    if (!var) return ENVIL_CONFIG_ERROR;
    
    logger(LOG_TRACE, "Validating variable '%s' with value '%s'", var->name, value ? value : "NULL");
    logger(LOG_TRACE, "Variable details:");
    logger(LOG_TRACE, "  name: %s", var->name);
    logger(LOG_TRACE, "  type: %s", get_type_name(var->type));
    logger(LOG_TRACE, "  required: %s", var->required ? "true" : "false");
    logger(LOG_TRACE, "  default: %s", var->default_value ? var->default_value : "NULL");
    logger(LOG_TRACE, "  num_checks: %d", var->check_count);
    
    for (int i = 0; i < var->check_count; i++) {
        const Check *check = &var->checks[i];
        logger(LOG_TRACE, "  check[%d]: %s", i, check->definition->name);
    }

    // Check if variable exists or has default
    if (!value) {
        if (var->required) {
            logger(LOG_INFO, "Error: Variable '%s' is required but not set", var->name);
            return ENVIL_MISSING_VAR;
        }
        logger(LOG_INFO, "Variable '%s' not set but not required, validation passed", var->name);
        return ENVIL_OK;
    }

    logger(LOG_INFO, "Checking value: '%s'", value);

    // Run type check first if present
    for (int i = 0; i < var->check_count; i++) {
        const Check *check = &var->checks[i];
        if (strcmp(check->definition->name, "type") == 0) {
            logger(LOG_INFO, "Running type check: %s", get_type_name(check->value.int_value));
            int check_result = validate_check(check, value);
            if (check_result != ENVIL_OK) {
                logger(LOG_INFO, "Error: Variable '%s' has invalid type - expected %s", 
                    var->name, get_type_name(check->value.int_value));
                return check_result;
            }
            logger(LOG_INFO, "Type check passed");
        }
    }

    // Run remaining checks
    for (int i = 0; i < var->check_count; i++) {
        const Check *check = &var->checks[i];
        if (strcmp(check->definition->name, "type") != 0) {
            logger(LOG_INFO, "Running check: %s", check->definition->name);
            
            int check_result = validate_check(check, value);
            if (check_result != ENVIL_OK) {
                logger(LOG_INFO, "Error: Variable '%s' failed %s check", var->name, check->definition->name);
                
                if (strcmp(check->definition->name, "gt") == 0) {
                    logger(LOG_INFO, " (value must be greater than %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "lt") == 0) {
                    logger(LOG_INFO, " (value must be less than %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "eq") == 0) {
                    logger(LOG_INFO, " (value must be equal to %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "ne") == 0) {
                    logger(LOG_INFO, " (value must not be equal to %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "len") == 0) {
                    logger(LOG_INFO, " (length must be exactly %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "lengt") == 0) {
                    logger(LOG_INFO, " (length must be greater than %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "lenlt") == 0) {
                    logger(LOG_INFO, " (length must be less than %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "regex") == 0) {
                    logger(LOG_INFO, " (value must match pattern: %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "enum") == 0) {
                    logger(LOG_INFO, " (allowed values: ");
                    char **values = check->value.enum_values;
                    while (*values) {
                        logger(LOG_INFO, "%s", *values);
                        if (*(values + 1)) logger(LOG_INFO, ", ");
                        values++;
                    }
                    logger(LOG_INFO, ")");
                }
                logger(LOG_INFO, "");
                return check_result;
            }
            logger(LOG_INFO, "Check passed: %s", check->definition->name);
        }
    }

    logger(LOG_INFO, "All validations passed for '%s'", var->name);
    return ENVIL_OK;
}

int validate_variable_with_errors(const EnvVariable *var, const char *value, ValidationErrors* errors) {
    if (!var || !errors) return ENVIL_CONFIG_ERROR;
    
    logger(LOG_INFO, "Validating variable '%s' with value '%s'", var->name, value ? value : "NULL");
    const char *type_str = NULL;

    ///
    logger(LOG_INFO, "Number of checks: %d", var->check_count);
    for (int i = 0; i < var->check_count; i++) {
        const Check *check = &var->checks[i];
        logger(LOG_INFO, "Check: %s", check->definition->name);
        if (strcmp(check->definition->name, "type") == 0) {
            type_str = get_type_name(check->value.int_value);
            logger(LOG_INFO, "Variable '%s' has type check: %s", var->name, type_str);
            break;
        }
    }
    ///

    // Check if variable exists or has default
    if (!value) {
        if (var->required) {
            add_validation_error(errors, var->name, "variable is required but not set", ENVIL_MISSING_VAR);
            return ENVIL_MISSING_VAR;
        }
        logger(LOG_INFO, "Variable '%s' not set but not required, validation passed", var->name);
        return ENVIL_OK;
    }

    logger(LOG_INFO, "Checking value: '%s'", value);

    // Run all checks, with type check first if present
    for (int i = 0; i < var->check_count; i++) {
        const Check *check = &var->checks[i];
        
        // Run type check first
        if (strcmp(check->definition->name, "type") == 0) {
            logger(LOG_INFO, "Running type check: %s", get_type_name(check->value.int_value));
            int check_result = validate_check(check, value);
            if (check_result != ENVIL_OK) {
                char message[256];
                snprintf(message, sizeof(message), "invalid type - expected %s\n", get_type_name(check->value.int_value));
                add_validation_error(errors, var->name, message, check_result);
                return check_result;
            }
            logger(LOG_INFO, "Type check passed");
        }
    }

    // Run remaining checks
    for (int i = 0; i < var->check_count; i++) {
        const Check *check = &var->checks[i];
        if (strcmp(check->definition->name, "type") != 0) {
            logger(LOG_INFO, "Running check: %s", check->definition->name);
            int check_result = validate_check(check, value);
            if (check_result != ENVIL_OK) {
                char message[512] = {0};
                snprintf(message, sizeof(message), "failed %s check", check->definition->name);
                
                if (strcmp(check->definition->name, "gt") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (value must be greater than %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "lt") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (value must be less than %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "eq") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (value must be equal to %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "ne") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (value must not be equal to %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "len") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (length must be exactly %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "lengt") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (length must be greater than %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "lenlt") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (length must be less than %d)", check->value.int_value);
                } else if (strcmp(check->definition->name, "regex") == 0) {
                    snprintf(message + strlen(message), sizeof(message) - strlen(message), " (value must match pattern: %s)", check->value.str_value);
                } else if (strcmp(check->definition->name, "enum") == 0) {
                    strcat(message, " (allowed values: ");
                    char **values = check->value.enum_values;
                    while (*values) {
                        strcat(message, *values);
                        if (*(values + 1)) strcat(message, ", ");
                        values++;
                    }
                    strcat(message, ")");
                }
                
                add_validation_error(errors, var->name, message, check_result);
                return check_result;
            }
            logger(LOG_INFO, "Check passed: %s", check->definition->name);
        }
    }

    logger(LOG_INFO, "All validations passed for '%s'", var->name);
    return ENVIL_OK;
}

char *get_env_value(const EnvVariable *var) {
    if (!var || !var->name) return NULL;

    char *value = getenv(var->name);
    if (!value && var->default_value) {
        value = var->default_value;
    }

    return value ? strdup(value) : NULL;
}


ValidationErrors* create_validation_errors(void) {
    ValidationErrors* errors = malloc(sizeof(ValidationErrors));
    if (!errors) return NULL;
    
    errors->errors = malloc(INITIAL_ERROR_CAPACITY * sizeof(ValidationError));
    if (!errors->errors) {
        free(errors);
        return NULL;
    }
    
    errors->count = 0;
    errors->capacity = INITIAL_ERROR_CAPACITY;
    return errors;
}

void add_validation_error(ValidationErrors* errors, const char* var_name, const char* message, int error_code) {
    if (!errors) return;
    
    // Grow array if needed
    if (errors->count >= errors->capacity) {
        int new_capacity = errors->capacity * 2;
        ValidationError* new_errors = realloc(errors->errors, new_capacity * sizeof(ValidationError));
        if (!new_errors) return;
        
        errors->errors = new_errors;
        errors->capacity = new_capacity;
    }
    
    // Add new error
    ValidationError* error = &errors->errors[errors->count++];
    error->name = strdup(var_name);
    error->message = strdup(message);
    error->error_code = error_code;
}

void free_validation_errors(ValidationErrors* errors) {
    if (!errors) return;
    
    for (int i = 0; i < errors->count; i++) {
        free((void*)errors->errors[i].name);
        free((void*)errors->errors[i].message);
    }
    free(errors->errors);
    free(errors);
}
