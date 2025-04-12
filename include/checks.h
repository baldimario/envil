#ifndef ENVIL_CHECKS_H
#define ENVIL_CHECKS_H

#include "types.h"

int check_mock(const char* value, const void* param);
int check_cmd(const char* value, const void* cmd);
int check_type(const char* value, const void* type_ptr);
int check_gt(const char* value, const void* threshold);
int check_lt(const char* value, const void* threshold);
int check_len(const char* value, const void* length);
int check_enum(const char* value, const void* values);
int check_cmd(const char* value, const void* cmd);
const CheckDefinition* register_check(const char* name, const char* description, CheckFunction check_fn);
const CheckDefinition* get_check_definition(const char* name);
const CheckDefinition* get_check_definition_by_index(int index);

#endif // ENVIL_CHECKS_H