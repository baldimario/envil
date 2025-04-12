#ifndef ENVIL_VALIDATOR_H
#define ENVIL_VALIDATOR_H

#include "types.h"

// Return codes
#define ENVIL_OK 0
#define ENVIL_CONFIG_ERROR 1
#define ENVIL_MISSING_VAR 2
#define ENVIL_TYPE_ERROR 3
#define ENVIL_VALUE_ERROR 4
#define ENVIL_CUSTOM_ERROR 5

// Validation functions
int validate_variable(const EnvVariable *var, const char *value);
int validate_type(EnvType type, const char *value);
int validate_check(const Check *check, const char *value);
char *get_env_value(const EnvVariable *var);

// New validation function with error collection
int validate_variable_with_errors(const EnvVariable *var, const char *value, ValidationErrors* errors);

// Utility functions
bool is_integer(const char *str);
bool is_json(const char *str);
bool is_float(const char *str);

// Error handling functions
ValidationErrors* create_validation_errors(void);
void add_validation_error(ValidationErrors* errors, const char* var_name, const char* message, int error_code);
void free_validation_errors(ValidationErrors* errors);

#endif // ENVIL_VALIDATOR_H