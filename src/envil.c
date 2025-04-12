#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "argparse.h"

int main(int argc, char **argv) {
    printf("Envil.\n");
    int option, option_index = 0;

    struct option* long_options = create_long_options();

    while ((option = getopt_long(argc, argv, get_getopt_long_string(long_options), long_options, &option_index)) != -1) {
        switch (option) {
        case 'c':
        case 'e':
        case 'd':
        case 'p':
        case 'l':
            list_checks();
            break;
        case 'v':
            printf("Option: %c\n", option);
            break;
        case 'h':
        case '?':
            print_usage();
            break;
        default:
            break;
        }
    }
    
    return 0;
}