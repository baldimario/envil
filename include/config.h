#ifndef ENVIL_CONFIG_H
#define ENVIL_CONFIG_H

#include <getopt.h>
#include <stddef.h>
#include "types.h"
#include "checks.h"

extern struct option check_options[];
extern CheckDefinition checks[];
extern struct option base_options[];

size_t get_base_options_count();
size_t get_check_options_count();
size_t get_options_count();

#endif // ENVIL_CONFIG_H