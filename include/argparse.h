#ifndef ENVIL_ARGPARSE_H
#define ENVIL_ARGPARSE_H

#include <getopt.h>
#include "checks.h"
#include "types.h"
#include "config.h"

/**
 * @brief Creates option array for check-specific options
 * @param checks Array of check definitions
 * @param count Number of checks in the array
 * @return Dynamically allocated array of struct option, must be freed by caller
 */
struct option* get_check_options(CheckDefinition checks[], int count);

/**
 * @brief Creates combined array of all long options (base + checks)
 * @return Dynamically allocated array of struct option, must be freed by caller
 */
struct option* create_long_options(void);

/**
 * @brief Creates getopt_long option string for base options
 * @return Dynamically allocated string, must be freed by caller
 */
char* get_getopt_long_string(void);

/**
 * @brief Lists all available checks and their descriptions
 */
void list_checks(void);

/**
 * @brief Prints usage information and exits with status code 1
 */
void print_usage(void);

#endif // ENVIL_ARGPARSE_H