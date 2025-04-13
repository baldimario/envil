#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "types.h"
#include "config.h"
#include "validator.h"
#include "checks.h"
#include "logger.h"

int check_mock(const char* value, const void* param) {
    printf("Mock check called with value: %s and param: %s\n", value, (char*) param);
    return 0;
}


#define MAX_CHECKS 32

static struct {
    CheckDefinition definitions[MAX_CHECKS];
    int count;
} registry = {0};

// Helper function for numeric comparisons
static bool parse_numeric_value(const char* value, double* result) {
    if (!value) return false;
    char* endptr;
    *result = strtod(value, &endptr);
    return *endptr == '\0';
}

// Built-in check functions
int check_type(const char* value, const void* type_ptr) {
    EnvType type = *(EnvType*)type_ptr;
    return validate_type(type, value);
}

int check_gt(const char* value, const void* threshold) {
    double val, threshold_val = *(int*)threshold;
    if (!parse_numeric_value(value, &val)) {
        return ENVIL_VALUE_ERROR;
    }
    return val > threshold_val ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_lt(const char* value, const void* threshold) {
    double val, threshold_val = *(int*)threshold;
    if (!parse_numeric_value(value, &val)) {
        return ENVIL_VALUE_ERROR;
    }
    return val < threshold_val ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_len(const char* value, const void* length) {
    return strlen(value) == *(size_t*)length ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_enum(const char* value, const void* values) {
    if (!value || !values) return ENVIL_VALUE_ERROR;
    
    const char** enum_values = (const char**)values;
    while (*enum_values) {
        if (strcmp(value, *enum_values) == 0) {
            return ENVIL_OK;
        }
        enum_values++;
    }
    return ENVIL_VALUE_ERROR;
}

int check_cmd(const char* value, const void* cmd_data) {
    if (!value || !cmd_data) return ENVIL_CUSTOM_ERROR;
    
    const struct {
        char* cmd;
        size_t cmd_len;
    } *cmd_value = cmd_data;

    if (!cmd_value->cmd || !cmd_value->cmd_len) {  // Check for empty command
        fprintf(stderr, "Empty command provided\n");
        return ENVIL_CUSTOM_ERROR;
    }

    // Print command for debugging
    logger(LOG_INFO, "Executing command: %s", cmd_value->cmd);
    
    int pipefd[2];
    pid_t pid;
    int status;

    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Failed to create pipe: %s\n", strerror(errno));
        return ENVIL_CUSTOM_ERROR;
    }

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return ENVIL_CUSTOM_ERROR;
    }

    if (pid == 0) {  // Child process
        // Set up environment
        setenv("VALUE", value, 1);
        
        // Close stdout and stderr before redirecting
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        // Redirect stderr to stdout
        dup2(pipefd[1], STDERR_FILENO);
        
        // Redirect stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        
        // Close unused pipe ends
        close(pipefd[0]);

        // Execute command through shell with proper args
        execl("/bin/sh", "sh", "-c", cmd_value->cmd, (char*)NULL);
        
        // If we get here, exec failed
        perror("Failed to execute command");
        _exit(1);  // Use _exit() in child process
    }

    // Parent process continues here
    close(pipefd[1]);  // Close write end

    // Read command output
    char buf[1024];
    ssize_t n;
    
    while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        fprintf(stderr, "Command output: %s", buf);
    }
    
    close(pipefd[0]);

    // Wait for child and get status
    if (waitpid(pid, &status, 0) == -1) {
        fprintf(stderr, "Wait failed: %s\n", strerror(errno));
        return ENVIL_CUSTOM_ERROR;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        logger(LOG_INFO, "Command exited with status: %d", exit_code);
        return exit_code == 0 ? ENVIL_OK : ENVIL_CUSTOM_ERROR;
    }

    fprintf(stderr, "Command did not exit normally\n");
    return ENVIL_CUSTOM_ERROR;
}

// Initialize built-in checks
__attribute__((constructor))
static void init_registry(void) {
    register_check("type", "Validates value matches specified type (string|integer|json|float)", check_type);
    register_check("gt", "Checks if numeric value is greater than specified threshold", check_gt);
    register_check("lt", "Checks if numeric value is less than specified threshold", check_lt);
    register_check("len", "Validates string length matches exactly", check_len);
    register_check("enum", "Validates value is one of specified options", check_enum);
    register_check("cmd", "Runs custom shell command for validation", check_cmd);
}

const CheckDefinition* register_check(const char* name, const char* description, CheckFunction check_fn) {
    if (registry.count >= MAX_CHECKS) return NULL;
    
    CheckDefinition* def = &registry.definitions[registry.count++];
    def->name = name;
    def->description = description;
    def->callback = check_fn;
    
    return def;
}

const CheckDefinition* get_check_definition(const char* name) {
    for (int i = 0; i < registry.count; i++) {
        if (strcmp(registry.definitions[i].name, name) == 0) {
            return &registry.definitions[i];
        }
    }
    return NULL;
}

const CheckDefinition* get_check_definition_by_index(int index) {
    if (index < 0 || index >= registry.count) return NULL;
    return &registry.definitions[index];
}

void list_available_checks(void) {
    printf("Available checks:\n\n");
    for (int i = 0; i < registry.count; i++) {
        printf("  %-10s %s\n", registry.definitions[i].name, registry.definitions[i].description);
    }
}