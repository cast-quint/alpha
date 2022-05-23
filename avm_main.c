/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * avm_main.c
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory-management.h"
#include "avm_util.h"

int main(int argc, char **argv) {
    if (argc == 1) {
        fgets(bytecode_filename, 256, stdin);
        bytecode_file = myfopen(bytecode_filename, "r");
    } else if (argc == 2) {
       bytecode_file = myfopen(argv[1], "r");
       strncpy(bytecode_filename, argv[1], 256);
    } else {
        puts("usage: avm <BYTECODE>");
        return EXIT_FAILURE;
    }

    if (!bytecode_file) {
        return EXIT_FAILURE;
    }

    avm_execute();

    return 0;
}