#ifndef ENVIL_ARGPARSE_H
#define ENVIL_ARGPARSE_H

#include <getopt.h>
#include "checks.h"
#include "types.h"
#include "config.h"

extern struct option base_options[];
extern struct option check_options[];
extern CheckDefinition checks[];

struct option* get_check_options(CheckDefinition checks[], int count);
struct option* create_long_options();
char* get_getopt_long_string();
void list_checks();
void print_usage();

#endif // ENVIL_ARGPARSE_H