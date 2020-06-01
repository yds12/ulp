/*
 *
 *
 * Scope checker.
 *
 */

#ifndef SCOPER_H
#define SCOPER_H

#include <stdio.h>
#include "datast.h"

typedef struct stScoperState {
  FILE* file;
  char* filename;
} ScoperState;

ScoperState scoperState;

void scopeCheckerStart(FILE* file, char* filename, Node* ast);

/*
 * Looks from the specified node upwards in the tree to find a symbol that
 * matches the specified identifier token. 
 *
 * node: the node to start the search from (where the symbol is used).
 * symToken: the identifier token containing the name of the symbol.
 * returns: the nearest symbol that matches the token name, if any. Otherwise,
 *   returns NULL.
 *
 */
Symbol* lookupSymbol(Node* node, Token* symToken);

#endif

