/*
 * sdf.h
 * By Steven Smith
 */

#ifndef SDF_H_
#define SDF_H_

typedef struct sdfNode {
    char *data;
    struct sdfNode *next;
    struct sdfNode *child;
} sdfNode;

sdfNode * parseSdfFile( const char *filename );

void printSdfFile( sdfNode *tree, FILE *fp );

void freeSdfFile( sdfNode *tree );

sdfNode * getChildren( sdfNode *tree, char *value );

#endif
