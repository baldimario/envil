#ifndef ENVIL_CONFIG_H
#define ENVIL_CONFIG_H

#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "types.h"
#include "checks.h"

extern struct option check_options[];
extern CheckDefinition checks[];
extern struct option base_options[];

// Base configuration functions
size_t get_base_options_count();
size_t get_check_options_count();
size_t get_options_count();

// Configuration file handling functions
int handle_config_option(const char* config_path, bool print_value);
int handle_yaml_config(FILE* config_file, bool print_value, ValidationErrors* errors);
int handle_json_config(FILE* config_file, bool print_value, ValidationErrors* errors);

// Environment variable validation helper
int validate_and_print_env(const char* var_name, const char* env_value, 
                          const char* default_value, bool print_value,
                          Check* checks, int check_count,
                          ValidationErrors* errors);

#endif // ENVIL_CONFIG_H