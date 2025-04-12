#ifndef ENVIL_TYPES_H
#define ENVIL_TYPES_H

#include <stdbool.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_NONE
} LogLevel;

typedef enum {
    TYPE_STRING,
    TYPE_INTEGER,
    TYPE_BOOLEAN,
    TYPE_FLOAT,
    TYPE_JSON
} EnvType;

typedef int (*CheckFunction)(char *value, char *param);

typedef struct {
    char *name;
    char *description;
    int (*callback)(char *value, char *param);
} CheckDefinition;

typedef struct {
    const CheckDefinition* check;
    union {
        int int_value;
        char* str_value;
        char** enum_values;
        char* cmd;
        void* custom_value;
    } value;
} Check;

typedef struct {
    char *name;
    char *default_value;
    bool required;
    EnvType type;
    Check* checks;
    int check_count;
} EnvVariable;

typedef struct {
    EnvVariable *variables;
    int variable_count; 
} Config;

typedef struct {
    char *name;
    char *message;
    int error_code;
} ValidationError;

typedef struct {
    ValidationError *errors;
    int error_count;
    int error_capacity;
} ValidationErrors;

typedef enum {
    CONFIG_YAML,
    CONFIG_JSON
} ConfigFormat;

extern LogLevel g_log_level;

#endif // ENVIL_TYPES_H