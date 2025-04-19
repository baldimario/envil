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
    {"ge", required_argument, 0, 0},
    {"le", required_argument, 0, 0},
    {"lengt", required_argument, 0, 0},
    {"lenlt", required_argument, 0, 0},
    {"regex", required_argument, 0, 0},
};

CheckDefinition checks[] = {
    {"type", "Check the type of the variable (integer,string,json,float,boolean)", check_type, NULL, 1, "Invalid type"},
    {"gt", "Check if greater than a value", check_gt, NULL, 1, "Invalid length"},
    {"lt", "Check if less than a value", check_lt, NULL, 1, "Invalid length"},
    {"enum", "Check if in a set of values (foo,bar,baz)", check_enum, NULL, 1, "Invalid enum value"},
    {"len", "Check the length of the variable", check_len, NULL, 1, "Invalid length"},
    {"cmd", "Run a command to validate the variable", check_cmd, NULL, 1, "Invalid command"},
    {"eq", "Check if equal to a value", check_eq, NULL, 1, "Invalid target"},
    {"ne", "Check if not equal to a value", check_ne, NULL, 1, "Invalid target"},
    {"ge", "Check if greater than or equal to a value", check_ge, NULL, 1, "Invalid length"},
    {"le", "Check if less than or equal to a value", check_le, NULL, 1, "Invalid length"},
    {"lengt", "Check if string length is greater than specified length", check_lengt, NULL, 1, "Invalid length"},
    {"lenlt", "Check if string length is less than specified length", check_lenlt, NULL, 1, "Invalid length"},
    {"regex", "Check if value matches regular expression pattern", check_regex, NULL, 1, "Invalid pattern"},
};

struct option base_options[] = {
    {"config", required_argument, 0, 'c'},
    {"env", required_argument, 0, 'e'},
    {"default", required_argument, 0, 'd'},
    {"print", no_argument, 0, 'p'},
    {"list-checks", no_argument, 0, 'l'},
    {"verbose", no_argument, 0, 'v'},
    {"completion", required_argument, 0, 'C'},
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