#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <regex.h>
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

typedef struct {
    CheckDefinition definitions[MAX_CHECKS];
    int count;
} CheckRegistry;

static CheckRegistry registry = {0};

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

int check_eq(const char* value, const void* target) {
    if (!value || !target) return ENVIL_VALUE_ERROR;
    return strcmp(value, (const char*)target) == 0 ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_ne(const char* value, const void* target) {
    if (!value || !target) return ENVIL_VALUE_ERROR;
    return strcmp(value, (const char*)target) != 0 ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_ge(const char* value, const void* threshold) {
    double val, threshold_val = *(int*)threshold;
    if (!parse_numeric_value(value, &val)) {
        return ENVIL_VALUE_ERROR;
    }
    return val >= threshold_val ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_le(const char* value, const void* threshold) {
    double val, threshold_val = *(int*)threshold;
    if (!parse_numeric_value(value, &val)) {
        return ENVIL_VALUE_ERROR;
    }
    return val <= threshold_val ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_lengt(const char* value, const void* length) {
    return strlen(value) > *(size_t*)length ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_lenlt(const char* value, const void* length) {
    return strlen(value) < *(size_t*)length ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

int check_regex(const char* value, const void* pattern) {
    if (!value || !pattern) return ENVIL_VALUE_ERROR;
    
    regex_t regex;
    int ret = regcomp(&regex, (const char*)pattern, REG_EXTENDED | REG_NOSUB);
    if (ret != 0) {
        char error_buf[100];
        regerror(ret, &regex, error_buf, sizeof(error_buf));
        logger(LOG_ERROR, "Failed to compile regex: %s", error_buf);
        return ENVIL_VALUE_ERROR;
    }
    
    ret = regexec(&regex, value, 0, NULL, 0);
    regfree(&regex);
    
    return ret == 0 ? ENVIL_OK : ENVIL_VALUE_ERROR;
}

// Initialize built-in checks
__attribute__((constructor))
static void init_registry(void) {
    int checks_count = get_check_options_count();
    for (int i = 0; i < checks_count; i++) {
        const CheckDefinition* def = get_check_definition_by_index(i);
        if (def) {
            register_check(def->name, def->description, def->callback, def->custom_data, def->has_arg, def->error_message);
        }
    }
}

const CheckDefinition* register_check(const char* name, const char* description, CheckFunction check_fn, void* custom_data, int has_arg, const char* error_message) {
    if (registry.count >= MAX_CHECKS) return NULL;
    
    CheckDefinition* def = &registry.definitions[registry.count++];
    def->name = name;
    def->description = description;
    def->callback = check_fn;
    def->custom_data = custom_data;
    def->has_arg = has_arg;
    def->error_message = error_message;
    
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