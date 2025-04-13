#include "types.h"

// Define the global log level with default value
LogLevel g_log_level = LOG_ERROR;

const char* get_type_name(EnvType type) {
    switch (type) {
        case TYPE_STRING:
            return "string";
        case TYPE_INTEGER:
            return "integer";
        case TYPE_BOOLEAN:
            return "boolean";
        case TYPE_FLOAT:
            return "float";
        case TYPE_JSON:
            return "json";
        default:
            return "unknown";
    }
}