
#ifndef AST_H
#define AST_H

#include "util.h"
#include "datast.h"

Node* astFirstLeaf(Node* ast);
Node* astLastLeaf(Node* ast);

// Prints the AST in GraphViz format
void graphvizAst(Node* ast);

// Checks if the AST looks healthy (memory-wise)
void checkTree(Node* node, int nodeCount);

void postorderTraverse(Node* node, void (*visit)(Node*));

#endif

