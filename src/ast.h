
#ifndef AST_H
#define AST_H

#include "parser.h"

Node* astFirstLeaf(Node* ast);
Node* astLastLeaf(Node* ast);

// Prints the AST in GraphViz format
void graphvizAst(Node* ast);

#endif

