#ifndef ENVIL_TYPES_H
#define ENVIL_TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LOG_NONE,      // No logging
    LOG_ERROR,     // Error messages only
    LOG_WARNING,   // Warnings and errors
    LOG_INFO,      // Standard info, warnings, and errors
    LOG_DEBUG,     // Detailed debug info
    LOG_TRACE      // Most verbose level
} LogLevel;

typedef enum {
    TYPE_STRING,
    TYPE_INTEGER,
    TYPE_BOOLEAN,
    TYPE_FLOAT,
    TYPE_JSON
} EnvType;

typedef int (*CheckFunction)(const char* value, const void* check_value);

typedef struct {
    const char* name;
    const char* description;
    CheckFunction callback;
    void* custom_data;
    int has_arg;
    const char* error_message;
} CheckDefinition;

typedef struct {
    const CheckDefinition* definition;
    union {
        int int_value;
        char* str_value;
        char** enum_values;
        struct {
            char* cmd;
            size_t cmd_len;
        } cmd_value;
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
    int count;
    int capacity;
} ValidationErrors;

typedef enum {
    CONFIG_YAML,
    CONFIG_JSON
} ConfigFormat;

extern LogLevel g_log_level;

const char* get_type_name(EnvType type);
const char* get_error_message(const CheckDefinition* check_def);

#endif // ENVIL_TYPES_H