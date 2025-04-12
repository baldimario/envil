#include <stdio.h>
#include "checks.h"

int check_mock(char *value, char *param) {
    printf("Mock check called with value: %s and param: %s\n", value, param);
    return 0;
}
