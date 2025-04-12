#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "argparse.h"
#include "types.h"

void test_create_long_options() {
    struct option* options = create_long_options();
    assert(options != NULL && "Options should not be NULL");
    
    // Test some known options exist
    int i = 0;
    while (options[i].name != NULL) {
        if (strcmp(options[i].name, "help") == 0) {
            assert(options[i].has_arg == no_argument);
            assert(options[i].val == 'h');
        }
        if (strcmp(options[i].name, "env") == 0) {
            assert(options[i].has_arg == required_argument);
            assert(options[i].val == 'e');
        }
        i++;
    }
    
    free(options);
    printf("test_create_long_options: PASS\n");
}

void test_getopt_string() {
    char* getopt_str = get_getopt_long_string();
    assert(getopt_str != NULL && "Getopt string should not be NULL");
    
    // Test that required options are present
    assert(strchr(getopt_str, 'h') != NULL && "Help option missing");
    assert(strchr(getopt_str, 'e') != NULL && "Env option missing");
    assert(strchr(getopt_str, 'c') != NULL && "Config option missing");
    
    // Test that options requiring arguments have colons
    char* env_pos = strchr(getopt_str, 'e');
    assert(env_pos != NULL && *(env_pos + 1) == ':' && "Env option should require an argument");
    
    free(getopt_str);
    printf("test_getopt_string: PASS\n");
}

void test_memory_safety() {
    // Test multiple allocations and frees
    for (int i = 0; i < 100; i++) {
        struct option* options = create_long_options();
        assert(options != NULL);
        free(options);
        
        char* getopt_str = get_getopt_long_string();
        assert(getopt_str != NULL);
        free(getopt_str);
    }
    printf("test_memory_safety: PASS\n");
}

int main() {
    printf("Running argparse tests...\n");
    
    test_create_long_options();
    test_getopt_string();
    test_memory_safety();
    
    printf("All tests passed!\n");
    return 0;
}