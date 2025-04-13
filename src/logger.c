#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "logger.h"
#include "types.h"

void logger(LogLevel level, const char* message, ...) {
    if (level > g_log_level) return;  // Only log if current level is verbose enough
    
    // Add prefix based on log level
    const char* prefix;
    switch (level) {
        case LOG_ERROR:
            prefix = "ERROR: ";
            break;
        case LOG_WARNING:
            prefix = "WARNING: ";
            break;
        case LOG_INFO:
            prefix = "INFO: ";
            break;
        case LOG_DEBUG:
            prefix = "DEBUG: ";
            break;
        case LOG_TRACE:
            prefix = "TRACE: ";
            break;
        default:
            prefix = "";
            break;
    }
    
    fprintf(stderr, "%s", prefix);
    
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}