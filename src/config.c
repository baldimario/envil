#include <stdio.h>
#include "config.h"

struct option check_options[] = {
    {"type", required_argument, 0, 0},
    {"gt", required_argument, 0, 0},
    {"lt", required_argument, 0, 0},
    {"enum", required_argument, 0, 0},
    {"len", required_argument, 0, 0},
    {"cmd", required_argument, 0, 0},
    {"eq", required_argument, 0, 0},
    {"ne", required_argument, 0, 0},
};

CheckDefinition checks[] = {
    {"type", "Check the type of the variable (integer,string,json,float,boolean)", check_type},
    {"gt", "Check if greater than a value", check_gt},
    {"lt", "Check if less than a value", check_lt},
    {"enum", "Check if in a set of values (foo,bar,baz)", check_enum},
    {"len", "Check the length of the variable", check_len},
    {"cmd", "Run a command to validate the variable", check_cmd},
    {"eq", "Check if equal to a value", check_mock},  // TODO: Implement check_eq
    {"ne", "Check if not equal to a value", check_mock},  // TODO: Implement check_ne
};

struct option base_options[] = {
    {"config", required_argument, 0, 'c'},
    {"env", required_argument, 0, 'e'},
    {"default", required_argument, 0, 'd'},
    {"print", no_argument, 0, 'p'},
    {"list-checks", no_argument, 0, 'l'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
};

size_t get_base_options_count() {
    return sizeof(base_options) / sizeof(struct option);
}

size_t get_check_options_count() {
    return sizeof(check_options) / sizeof(struct option);
}

size_t get_options_count() {
    return get_base_options_count() + get_check_options_count();
}