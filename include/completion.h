#ifndef ENVIL_COMPLETION_H
#define ENVIL_COMPLETION_H

#include <stdbool.h>

// Shell types supported for completion
typedef enum {
    SHELL_BASH,
    SHELL_ZSH,
    SHELL_UNKNOWN
} ShellType;

// Convert shell name to type
ShellType get_shell_type(const char* shell_name);

// Generate completion script for the specified shell
int generate_completion_script(ShellType shell, FILE* output);

#endif // ENVIL_COMPLETION_H
