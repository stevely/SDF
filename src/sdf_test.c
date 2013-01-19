/*
 * sdf_test.c
 * By Steven Smith
 */

#include <stdio.h>
#include "sdf.h"

int main(int argc, char **argv ) {
    sdfNode *tree;
    if( argc != 2 ) {
        printf("Usage: %s file\n", argv[0]);
        return 0;
    }
    tree = parseSdfFile(argv[1]);
    if( tree ) {
        printSdfFile(tree, stdout);
    }
    else {
        printf("Failed to parse SDF file!\n");
    }
    return 0;
}
