/*
 * sdf.c
 * By Steven Smith
 * Steve's Data Format
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sdf.h"

#define BUFFER_SIZE 512

typedef struct sdfStringList {
    char *data;
    struct sdfStringList *next;
} sdfStringList;

static char inputBuf[BUFFER_SIZE];
static int inputLoc = BUFFER_SIZE;
static int inputLeft = -1;
static char lineBuf[BUFFER_SIZE+1];

static int nextChar( FILE *fp, char *c ) {
    size_t i;
    if( !inputLeft ) {
        return 0;
    }
    else {
        if( inputLoc == BUFFER_SIZE ) {
            inputLoc = 0;
            i = fread(inputBuf, sizeof(char), BUFFER_SIZE, fp);
            if( i == 0 ) {
                inputLeft = 0;
                return 0;
            }
            else if( i < BUFFER_SIZE ) {
                inputLeft = i;
            }
            *c = inputBuf[inputLoc];
            inputLoc++;
            inputLeft--;
            return 1;
        }
        else {
            *c = inputBuf[inputLoc];
            inputLoc++;
            return 1;
        }
    }
}

static int getLine( FILE *fp ) {
    char c;
    int i, more;
    i = 0;
    while( i < BUFFER_SIZE ) {
        more = nextChar(fp, &c);
        if( !more ) {
            break;
        }
        /* Ignore line feed */
        else if( c == '\r' ) {
            continue;
        }
        lineBuf[i] = c;
        i++;
        if( c == '\n' ) {
            break;
        }
    }
    lineBuf[i] = '\0';
    return more;
}

static sdfStringList * getParsedLine( FILE *fp, int *indent ) {
    sdfStringList *result, *listEnd;
    int i, j, quote;
    result = NULL;
    quote = 0;
    getLine(fp);
    /* Step 1: Find our indentation */
    for( i = 0; lineBuf[i]; i++ ) {
        if( lineBuf[i] != ' ' ) {
            break;
        }
    }
    *indent = i - 1 > 0 ? i - 1 : 0;
    /* Step 3: Scan for quotes and escaped newlines */
    for( j = i; lineBuf[j]; j++ ) {
        if( lineBuf[j] == '"' ) {
            if( !quote ) {
                quote = 1;
            }
            else {
                /* Put quoted stuff in its own buffer */
                quote = 0;
                if( result == NULL ) {
                    result = listEnd = (sdfStringList*)malloc(sizeof(sdfStringList));
                }
                else {
                    listEnd->next = (sdfStringList*)malloc(sizeof(sdfStringList));
                    listEnd = listEnd->next;
                }
                listEnd->next = NULL;
                listEnd->data = (char*)malloc(sizeof(char) * (j - i + 1));
                memcpy(listEnd->data, lineBuf + i, sizeof(char) * (j - i));
                listEnd->data[j-i] = '\0';
                i = j + 1;
            }
        }
        else if( lineBuf[j] == '\n' ) {
            if( quote ) {
                /* We're quoted, so keep on keeping on */
                if( j - i == 1 && lineBuf[i] == '"' ) {
                    /* If it's nothing but a quote and a newline, skip the newline */
                    getLine(fp);
                    i = 0;
                    j = -1; /* j will be incremented at the top of the for loop */
                }
                else {
                    /* Create buffer for the line */
                    if( result == NULL ) {
                        result = listEnd = (sdfStringList*)malloc(sizeof(sdfStringList));
                    }
                    else {
                        listEnd->next = (sdfStringList*)malloc(sizeof(sdfStringList));
                        listEnd = listEnd->next;
                    }
                    listEnd->next = NULL;
                    listEnd->data = (char*)malloc(sizeof(char) * (j - i + 2));
                    memcpy(listEnd->data, lineBuf + i, sizeof(char) * (j - i + 1));
                    listEnd->data[j-i+1] = '\0';
                    getLine(fp);
                    i = 0;
                    j = -1; /* j will be incremented at the top of the for loop */
                }
            }
            else {
                /* Just a vanilla line, create buffer */
                if( result == NULL ) {
                    result = listEnd = (sdfStringList*)malloc(sizeof(sdfStringList));
                }
                else {
                    listEnd->next = (sdfStringList*)malloc(sizeof(sdfStringList));
                    listEnd = listEnd->next;
                }
                listEnd->next = NULL;
                listEnd->data = (char*)malloc(sizeof(char) * (j - i + 1));
                memcpy(listEnd->data, lineBuf + i, sizeof(char) * (j - i));
                listEnd->data[j-i] = '\0';
            }
        }
    }
    return result;
}

static char * compressString( sdfStringList *list ) {
    sdfStringList *l, *tmp;
    char *result, *c, *r;
    int size;
    if( !list ) {
        return NULL;
    }
    size = 0;
    /* Step 1: Get total size */
    for( l = list; l; l = l->next ) {
        for( c = l->data; *c; c++ ) {
            size++;
        }
    }
    /* Step 2: Create and populate */
    result = (char*)malloc(sizeof(char) * (size + 1));
    r = result;
    for( l = list; l; l = l->next ) {
        for( c = l->data; *c; c++ ) {
            *r = *c;
            r++;
        }
    }
    /* Step 3: Free old data */
    l = list;
    while( l ) {
        tmp = l;
        l = l->next;
        free(tmp);
    }
    *r = '\0';
    return result;
}

static char *currentLine = NULL;
static int currentIndent = 0;

static sdfNode * createTree( FILE *fp, int indent ) {
    sdfNode *result, *resultEnd;
    result = NULL;
    while( currentLine ) {
        if( currentIndent == indent ) {
            if( !result ) {
                result = resultEnd = (sdfNode*)malloc(sizeof(sdfNode));
            }
            else {
                resultEnd->next = (sdfNode*)malloc(sizeof(sdfNode));
                resultEnd = resultEnd->next;
            }
            resultEnd->child = NULL;
            resultEnd->next = NULL;
            resultEnd->data = currentLine;
            currentLine = compressString(getParsedLine(fp, &currentIndent));
        }
        else if( currentIndent > indent ) {
            /* In the case where the file starts with indentation, we simply
             * set that to the base indentation. */
            if( !result ) {
                result = resultEnd = createTree(fp, currentIndent);
            }
            else {
                resultEnd->child = createTree(fp, currentIndent);
            }
        }
        else if( currentIndent < indent ) {
            return result;
        }
    }
    return result;
}

sdfNode * parseSdfFile( const char *filename ) {
    FILE *fp;
    sdfNode *tree;
    fp = fopen(filename, "r");
    if( !fp ) {
        printf("ERROR: Failed opening file: %s\n", filename);
        return NULL;
    }
    currentLine = compressString(getParsedLine(fp, &currentIndent));
    tree = createTree(fp, 0);
    fclose(fp);
    return tree;
}

static void printSdfFile2( sdfNode *tree, FILE *fp, int indent ) {
    int i;
    while( tree ) {
        for( i = 0; i < indent; i++ ) {
            fprintf(fp, " ");
        }
        fprintf(fp, "%s\n", tree->data);
        printSdfFile2(tree->child, fp, indent+1);
        tree = tree->next;
    }
}

void printSdfFile( sdfNode *tree, FILE *fp ) {
    printSdfFile2(tree, fp, 0);
}

void freeSdfFile( sdfNode *tree ) {
    sdfNode *tmp;
    while( tree ) {
        freeSdfFile(tree->child);
        tmp = tree;
        tree = tree->next;
        free(tmp->data);
        free(tmp);
    }
}

sdfNode * getChildren( sdfNode *tree, char *value ) {
    while( tree ) {
        if( strcmp(tree->data, value) == 0 ) {
            return tree->child;
        }
        tree = tree->next;
    }
    return NULL;
}
