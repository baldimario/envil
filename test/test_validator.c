#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "validator.h"
#include "checks.h"
#include "types.h"

// Test validation of different types
void test_type_validation() {
    printf("Testing type validation...\n");
    
    // Test integer validation
    assert(validate_type(TYPE_INTEGER, "123") == ENVIL_OK);
    assert(validate_type(TYPE_INTEGER, "-123") == ENVIL_OK);
    assert(validate_type(TYPE_INTEGER, "12.3") == ENVIL_TYPE_ERROR);
    assert(validate_type(TYPE_INTEGER, "abc") == ENVIL_TYPE_ERROR);
    
    // Test float validation
    assert(validate_type(TYPE_FLOAT, "123.45") == ENVIL_OK);
    assert(validate_type(TYPE_FLOAT, "-123.45") == ENVIL_OK);
    assert(validate_type(TYPE_FLOAT, "123") == ENVIL_OK);
    assert(validate_type(TYPE_FLOAT, "abc") == ENVIL_TYPE_ERROR);
    
    // Test JSON validation
    assert(validate_type(TYPE_JSON, "{\"key\":\"value\"}") == ENVIL_OK);
    assert(validate_type(TYPE_JSON, "[1,2,3]") == ENVIL_OK);
    assert(validate_type(TYPE_JSON, "invalid json") == ENVIL_TYPE_ERROR);
    
    // Test string validation (should always pass)
    assert(validate_type(TYPE_STRING, "any string") == ENVIL_OK);
    assert(validate_type(TYPE_STRING, "123") == ENVIL_OK);
    assert(validate_type(TYPE_STRING, "") == ENVIL_OK);
    
    printf("Type validation tests passed!\n");
}

// Test individual check functions
void test_check_functions() {
    printf("Testing check functions...\n");
    
    // Test gt check
    int gt_val = 10;
    assert(check_gt("15", &gt_val) == ENVIL_OK);
    assert(check_gt("5", &gt_val) == ENVIL_VALUE_ERROR);
    assert(check_gt("abc", &gt_val) == ENVIL_VALUE_ERROR);
    
    // Test lt check
    int lt_val = 10;
    assert(check_lt("5", &lt_val) == ENVIL_OK);
    assert(check_lt("15", &lt_val) == ENVIL_VALUE_ERROR);
    assert(check_lt("abc", &lt_val) == ENVIL_VALUE_ERROR);
    
    // Test len check
    size_t len_val = 3;
    assert(check_len("abc", &len_val) == ENVIL_OK);
    assert(check_len("abcd", &len_val) == ENVIL_VALUE_ERROR);
    assert(check_len("ab", &len_val) == ENVIL_VALUE_ERROR);
    
    // Test enum check
    const char *enum_values[] = {"foo", "bar", "baz", NULL};
    assert(check_enum("foo", enum_values) == ENVIL_OK);
    assert(check_enum("bar", enum_values) == ENVIL_OK);
    assert(check_enum("invalid", enum_values) == ENVIL_VALUE_ERROR);
    
    printf("Check function tests passed!\n");
}

// Test check registry functionality
void test_check_registry() {
    printf("Testing check registry...\n");
    
    // Test getting built-in checks
    assert(get_check_definition("type") != NULL);
    assert(get_check_definition("gt") != NULL);
    assert(get_check_definition("lt") != NULL);
    assert(get_check_definition("len") != NULL);
    assert(get_check_definition("enum") != NULL);
    assert(get_check_definition("cmd") != NULL);
    
    // Test invalid check name
    assert(get_check_definition("invalid") == NULL);
    
    // Test registering a new check
    const CheckDefinition *new_check = register_check(
        "test_check",
        "Test check description",
        check_mock
    );
    assert(new_check != NULL);
    assert(strcmp(new_check->name, "test_check") == 0);
    assert(new_check->callback == check_mock);
    
    printf("Check registry tests passed!\n");
}

// Test complete variable validation
void test_variable_validation() {
    printf("Testing variable validation...\n");
    
    // Create a test variable with multiple checks
    EnvVariable var = {
        .name = "TEST_VAR",
        .type = TYPE_INTEGER,
        .required = true,
        .default_value = NULL,
        .checks = NULL,
        .check_count = 0
    };
    
    // Set up checks
    Check checks[3];
    
    // Type check
    checks[0].definition = get_check_definition("type");
    checks[0].value.int_value = TYPE_INTEGER;
    
    // Greater than check
    checks[1].definition = get_check_definition("gt");
    checks[1].value.int_value = 0;
    
    // Less than check
    checks[2].definition = get_check_definition("lt");
    checks[2].value.int_value = 100;
    
    var.checks = checks;
    var.check_count = 3;
    
    // Test validation
    ValidationErrors* errors;
    
    // Test valid value
    errors = create_validation_errors();
    assert(validate_variable_with_errors(&var, "50", errors) == ENVIL_OK);
    assert(errors->count == 0);
    free_validation_errors(errors);
    
    // Test value less than minimum
    errors = create_validation_errors();
    assert(validate_variable_with_errors(&var, "-1", errors) == ENVIL_VALUE_ERROR);
    assert(errors->count == 1);
    assert(strcmp(errors->errors[0].name, "TEST_VAR") == 0);
    assert(strstr(errors->errors[0].message, "greater than 0") != NULL);
    free_validation_errors(errors);
    
    // Test value greater than maximum
    errors = create_validation_errors();
    assert(validate_variable_with_errors(&var, "150", errors) == ENVIL_VALUE_ERROR);
    assert(errors->count == 1);
    assert(strcmp(errors->errors[0].name, "TEST_VAR") == 0);
    assert(strstr(errors->errors[0].message, "less than 100") != NULL);
    free_validation_errors(errors);
    
    // Test invalid type
    errors = create_validation_errors();
    assert(validate_variable_with_errors(&var, "abc", errors) == ENVIL_TYPE_ERROR);
    assert(errors->count == 1);
    assert(strcmp(errors->errors[0].name, "TEST_VAR") == 0);
    assert(strstr(errors->errors[0].message, "invalid type") != NULL);
    free_validation_errors(errors);
    
    // Test missing required value
    errors = create_validation_errors();
    assert(validate_variable_with_errors(&var, NULL, errors) == ENVIL_MISSING_VAR);
    assert(errors->count == 1);
    assert(strcmp(errors->errors[0].name, "TEST_VAR") == 0);
    assert(strstr(errors->errors[0].message, "required") != NULL);
    free_validation_errors(errors);
    
    printf("Variable validation tests passed!\n");
}

// Test validation errors collection
void test_validation_errors() {
    printf("Testing validation errors...\n");
    
    ValidationErrors* errors = create_validation_errors();
    assert(errors != NULL);
    assert(errors->count == 0);
    
    // Test adding errors
    add_validation_error(errors, "TEST_VAR", "test error message", ENVIL_TYPE_ERROR);
    assert(errors->count == 1);
    assert(strcmp(errors->errors[0].name, "TEST_VAR") == 0);
    assert(strcmp(errors->errors[0].message, "test error message") == 0);
    assert(errors->errors[0].error_code == ENVIL_TYPE_ERROR);
    
    // Test multiple errors
    add_validation_error(errors, "ANOTHER_VAR", "another error", ENVIL_VALUE_ERROR);
    assert(errors->count == 2);
    
    // Clean up
    free_validation_errors(errors);
    
    printf("Validation errors tests passed!\n");
}

int main() {
    printf("Running validator tests...\n\n");
    
    test_type_validation();
    test_check_functions();
    test_check_registry();
    test_variable_validation();
    test_validation_errors();
    
    printf("\nAll validator tests passed!\n");
    return 0;
}