/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * comp_main.c
 */
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "memory-management.h"
#include "symtable.h"
#include "util.h"
#include "parse.h"
#include "tcode.h"

extern FILE* yyin;
extern int g_syntax_error;

int main(int argc, char **argv) {

    int parse_val;
    g_log = FALSE;
    FILE* fbuff;

    switch (argc) {
        case 1: {

            yyin = stdin;
            g_input_filename = "stdin";
            break;
        }

        case 2: {
            if (strcmp(argv[1], "--log") == 0) {
                g_log = TRUE;
                mkdir("logs", S_IRWXU);
                break;
            }

            fbuff = myfopen(argv[1], "r");
            if (fbuff == NULL) {
                return EXIT_FAILURE;
            }

            g_input_filename = argv[1];
            yyin = fbuff;

            break;
        }

        case 3: {

            if (strcmp(argv[1], "--log") != 0) {
                fprintf(stderr, "compiler: Invalid option!\n\n");
                return EXIT_FAILURE;
            }

            g_log = TRUE;
            mkdir("logs", S_IRWXU);
            fbuff = myfopen(argv[2], "r");
            if (fbuff == NULL) {
                return EXIT_FAILURE;
            }

            g_input_filename = argv[2];
            yyin = fbuff;

            break;
        }

        default:
            fprintf(stderr, "compiler: usage: ac [--log] [INPUT_FILE]\n\n");
            return EXIT_FAILURE;
    }

    init();

    parse_val = yyparse();

    if (g_syntax_error) {
        cleanup();
        return EXIT_FAILURE;
    }

    generate_tcode();
    write_bytecode();
    if (g_log) {
        print_symtable();
        print_quads();
        print_bytecode();

        fprintf(stderr, "compiler: Check logs folder!\n\n");
    }

    if (argc > 1) {
        fclose(fbuff);
    }

    printf("%s", make_bytefilename());

    cleanup();

    return parse_val;
}
