/*
 * @file main.c
 * @author Akagi201
 * @date 2015/02/04
 */

#include <stdio.h>
#include <stdlib.h>
#include "flv-parser.h"

void usage(char *program_name) {
    printf("Usage: %s [input.flv]\n", program_name);
    exit(-1);
}

int main(int argc, char **argv) {

    FILE *infile = NULL;

    if (argc == 1) {
        infile = stdin;
    } else {
        infile = fopen(argv[1], "r");
        if (!infile) {
            usage(argv[0]);
        }
    }

    flv_parser_init(infile);

    flv_parser_run();

    printf("\nFinished analyzing\n");

    return 0;
}
