#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "checks.h"
#include "types.h"
#include "validator.h"

void test_check_type() {
    printf("Testing check_type...\n");
    
    // Test integer type
    EnvType type = TYPE_INTEGER;
    assert(check_type("123", &type) == ENVIL_OK);
    assert(check_type("-123", &type) == ENVIL_OK);
    assert(check_type("12.3", &type) == ENVIL_TYPE_ERROR);
    assert(check_type("abc", &type) == ENVIL_TYPE_ERROR);
    
    // Test float type
    type = TYPE_FLOAT;
    assert(check_type("123.45", &type) == ENVIL_OK);
    assert(check_type("-123.45", &type) == ENVIL_OK);
    assert(check_type("123", &type) == ENVIL_OK);
    assert(check_type("abc", &type) == ENVIL_TYPE_ERROR);
    
    // Test JSON type
    type = TYPE_JSON;
    assert(check_type("{\"key\":\"value\"}", &type) == ENVIL_OK);
    assert(check_type("[1,2,3]", &type) == ENVIL_OK);
    assert(check_type("{invalid}", &type) == ENVIL_TYPE_ERROR);
    
    // Test string type (should always pass)
    type = TYPE_STRING;
    assert(check_type("any string", &type) == ENVIL_OK);
    assert(check_type("123", &type) == ENVIL_OK);
    assert(check_type("", &type) == ENVIL_OK);
    
    printf("check_type tests passed!\n");
}

void test_check_gt() {
    printf("Testing check_gt...\n");
    
    // Test integer comparisons
    int threshold = 10;
    assert(check_gt("15", &threshold) == ENVIL_OK);
    assert(check_gt("5", &threshold) == ENVIL_VALUE_ERROR);
    assert(check_gt("10", &threshold) == ENVIL_VALUE_ERROR); // Equal should fail
    assert(check_gt("abc", &threshold) == ENVIL_VALUE_ERROR);
    
    // Test negative numbers
    threshold = -10;
    assert(check_gt("-5", &threshold) == ENVIL_OK);
    assert(check_gt("-15", &threshold) == ENVIL_VALUE_ERROR);
    
    // Test with zero threshold
    threshold = 0;
    assert(check_gt("1", &threshold) == ENVIL_OK);
    assert(check_gt("-1", &threshold) == ENVIL_VALUE_ERROR);
    
    printf("check_gt tests passed!\n");
}

void test_check_lt() {
    printf("Testing check_lt...\n");
    
    // Test integer comparisons
    int threshold = 10;
    assert(check_lt("5", &threshold) == ENVIL_OK);
    assert(check_lt("15", &threshold) == ENVIL_VALUE_ERROR);
    assert(check_lt("10", &threshold) == ENVIL_VALUE_ERROR); // Equal should fail
    assert(check_lt("abc", &threshold) == ENVIL_VALUE_ERROR);
    
    // Test negative numbers
    threshold = -10;
    assert(check_lt("-15", &threshold) == ENVIL_OK);
    assert(check_lt("-5", &threshold) == ENVIL_VALUE_ERROR);
    
    // Test with zero threshold
    threshold = 0;
    assert(check_lt("-1", &threshold) == ENVIL_OK);
    assert(check_lt("1", &threshold) == ENVIL_VALUE_ERROR);
    
    printf("check_lt tests passed!\n");
}

void test_check_len() {
    printf("Testing check_len...\n");
    
    // Test exact length matches
    size_t length = 3;
    assert(check_len("abc", &length) == ENVIL_OK);
    assert(check_len("123", &length) == ENVIL_OK);
    assert(check_len("ab", &length) == ENVIL_VALUE_ERROR);
    assert(check_len("abcd", &length) == ENVIL_VALUE_ERROR);
    
    // Test empty string
    length = 0;
    assert(check_len("", &length) == ENVIL_OK);
    assert(check_len("a", &length) == ENVIL_VALUE_ERROR);
    
    // Test longer string
    length = 10;
    assert(check_len("abcdefghij", &length) == ENVIL_OK);
    assert(check_len("abcdefghi", &length) == ENVIL_VALUE_ERROR);
    
    printf("check_len tests passed!\n");
}

void test_check_enum() {
    printf("Testing check_enum...\n");
    
    // Test basic enum values
    const char *values[] = {"foo", "bar", "baz", NULL};
    assert(check_enum("foo", values) == ENVIL_OK);
    assert(check_enum("bar", values) == ENVIL_OK);
    assert(check_enum("baz", values) == ENVIL_OK);
    assert(check_enum("invalid", values) == ENVIL_VALUE_ERROR);
    assert(check_enum("", values) == ENVIL_VALUE_ERROR);
    
    // Test single value enum
    const char *single[] = {"only", NULL};
    assert(check_enum("only", single) == ENVIL_OK);
    assert(check_enum("other", single) == ENVIL_VALUE_ERROR);
    
    // Test case sensitivity
    assert(check_enum("FOO", values) == ENVIL_VALUE_ERROR);
    assert(check_enum("Bar", values) == ENVIL_VALUE_ERROR);
    
    printf("check_enum tests passed!\n");
}

void test_check_cmd() {
    printf("Testing check_cmd...\n");
    
    // Test successful command
    assert(check_cmd("test", "exit 0") == ENVIL_OK);
    
    // Test failing command
    assert(check_cmd("test", "exit 1") == ENVIL_CUSTOM_ERROR);
    
    // Test command that checks the value
    assert(check_cmd("123", "test \"$VALUE\" = \"123\"") == ENVIL_OK);
    assert(check_cmd("456", "test \"$VALUE\" = \"123\"") == ENVIL_CUSTOM_ERROR);
    
    // Test invalid command
    assert(check_cmd("test", "nonexistentcommand") == ENVIL_CUSTOM_ERROR);
    
    printf("check_cmd tests passed!\n");
}

void test_check_registry() {
    printf("Testing check registry...\n");
    
    // Test built-in checks are registered
    assert(get_check_definition("type") != NULL);
    assert(get_check_definition("gt") != NULL);
    assert(get_check_definition("lt") != NULL);
    assert(get_check_definition("len") != NULL);
    assert(get_check_definition("enum") != NULL);
    assert(get_check_definition("cmd") != NULL);
    
    // Test invalid check name
    assert(get_check_definition("nonexistent") == NULL);
    
    // Test registering a new check
    CheckFunction dummy_fn = check_mock;
    const CheckDefinition *new_check = register_check(
        "test_check",
        "Test check description",
        dummy_fn,
        NULL,  // custom_data
        1      // has_arg
    );
    assert(new_check != NULL);
    assert(strcmp(new_check->name, "test_check") == 0);
    assert(new_check->callback == dummy_fn);
    
    // Verify the new check can be retrieved
    const CheckDefinition *retrieved = get_check_definition("test_check");
    assert(retrieved != NULL);
    assert(strcmp(retrieved->name, "test_check") == 0);
    assert(retrieved->callback == dummy_fn);
    
    printf("check registry tests passed!\n");
}

void test_check_eq() {
    printf("Testing check_eq...\n");
    
    // Test integer equality
    int value = 42;
    assert(check_eq("42", &value) == ENVIL_OK);
    assert(check_eq("41", &value) == ENVIL_VALUE_ERROR);
    assert(check_eq("43", &value) == ENVIL_VALUE_ERROR);
    assert(check_eq("abc", &value) == ENVIL_VALUE_ERROR);
    
    // Test negative numbers
    value = -10;
    assert(check_eq("-10", &value) == ENVIL_OK);
    assert(check_eq("-11", &value) == ENVIL_VALUE_ERROR);
    
    // Test zero
    value = 0;
    assert(check_eq("0", &value) == ENVIL_OK);
    assert(check_eq("-1", &value) == ENVIL_VALUE_ERROR);
    assert(check_eq("1", &value) == ENVIL_VALUE_ERROR);
    
    printf("check_eq tests passed!\n");
}

void test_check_ne() {
    printf("Testing check_ne...\n");
    
    // Test integer inequality
    int value = 42;
    assert(check_ne("41", &value) == ENVIL_OK);
    assert(check_ne("43", &value) == ENVIL_OK);
    assert(check_ne("42", &value) == ENVIL_VALUE_ERROR);
    assert(check_ne("abc", &value) == ENVIL_VALUE_ERROR);
    
    // Test negative numbers
    value = -10;
    assert(check_ne("-11", &value) == ENVIL_OK);
    assert(check_ne("-10", &value) == ENVIL_VALUE_ERROR);
    
    // Test zero
    value = 0;
    assert(check_ne("1", &value) == ENVIL_OK);
    assert(check_ne("-1", &value) == ENVIL_OK);
    assert(check_ne("0", &value) == ENVIL_VALUE_ERROR);
    
    printf("check_ne tests passed!\n");
}

void test_check_lengt() {
    printf("Testing check_lengt...\n");
    
    // Test length greater than
    size_t length = 3;
    assert(check_lengt("abcd", &length) == ENVIL_OK);
    assert(check_lengt("abcde", &length) == ENVIL_OK);
    assert(check_lengt("abc", &length) == ENVIL_VALUE_ERROR);
    assert(check_lengt("ab", &length) == ENVIL_VALUE_ERROR);
    
    // Test with zero length
    length = 0;
    assert(check_lengt("a", &length) == ENVIL_OK);
    assert(check_lengt("abc", &length) == ENVIL_OK);
    assert(check_lengt("", &length) == ENVIL_VALUE_ERROR);
    
    printf("check_lengt tests passed!\n");
}

void test_check_lenlt() {
    printf("Testing check_lenlt...\n");
    
    // Test length less than
    size_t length = 3;
    assert(check_lenlt("a", &length) == ENVIL_OK);
    assert(check_lenlt("ab", &length) == ENVIL_OK);
    assert(check_lenlt("abc", &length) == ENVIL_VALUE_ERROR);
    assert(check_lenlt("abcd", &length) == ENVIL_VALUE_ERROR);
    
    // Test with zero length
    length = 1;
    assert(check_lenlt("", &length) == ENVIL_OK);
    assert(check_lenlt("a", &length) == ENVIL_VALUE_ERROR);
    assert(check_lenlt("ab", &length) == ENVIL_VALUE_ERROR);
    
    printf("check_lenlt tests passed!\n");
}

void test_check_regex() {
    printf("Testing check_regex...\n");
    
    // Test basic patterns
    const char* pattern = "^foo.*$";
    assert(check_regex("foo", pattern) == ENVIL_OK);
    assert(check_regex("foobar", pattern) == ENVIL_OK);
    assert(check_regex("bar", pattern) == ENVIL_VALUE_ERROR);
    
    // Test more complex patterns
    pattern = "^[A-Za-z0-9]+$";
    assert(check_regex("abc123", pattern) == ENVIL_OK);
    assert(check_regex("ABC123", pattern) == ENVIL_OK);
    assert(check_regex("abc-123", pattern) == ENVIL_VALUE_ERROR);
    assert(check_regex("", pattern) == ENVIL_VALUE_ERROR);
    
    // Test email pattern
    pattern = "^[^@]+@[^@]+\\.[^@]+$";
    assert(check_regex("test@example.com", pattern) == ENVIL_OK);
    assert(check_regex("test.name@example.co.uk", pattern) == ENVIL_OK);
    assert(check_regex("invalid-email", pattern) == ENVIL_VALUE_ERROR);
    assert(check_regex("@example.com", pattern) == ENVIL_VALUE_ERROR);
    
    printf("check_regex tests passed!\n");
}

int main() {
    printf("Running check function tests...\n\n");
    
    test_check_type();
    test_check_gt();
    test_check_lt();
    test_check_len();
    test_check_enum();
    test_check_cmd();
    test_check_eq();
    test_check_ne();
    test_check_lengt();
    test_check_lenlt();
    test_check_regex();
    
    printf("\nAll check function tests passed!\n");
    return 0;
}