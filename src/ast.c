/*
 *
 *
 * The AST is built by the parser, but this code file is meant to be
 * responsible to navigate and manipulate the AST after (but also during)
 * the parsing.
 *
 */

#include <string.h>
#include "ast.h"

void graphvizAstRec(Node* node);

Node* astFirstLeaf(Node* ast) {
  Node* firstChild = ast;

  while(1) {
    if(firstChild->nChildren > 0) firstChild = firstChild->children[0];
    else break;
  }
  return firstChild;
}

Node* astLastLeaf(Node* ast) {
  Node* lastChild = ast;

  while(1) {
    if(lastChild->nChildren > 0) {
      lastChild = lastChild->children[lastChild->nChildren - 1];
    } else break;
  }
  return lastChild;
}

void graphvizAst(Node* ast) {
  printf("digraph G {\n");
  graphvizAstRec(ast);
  printf("}");
  printf("\n");
}

void graphvizAstRec(Node* node) {
  char* format = "%s";
  int len = strlen(format) + MAX_NODE_NAME;
  char nodeName[len];
  strReplaceNodeAbbrev(nodeName, format, node); 

  printf("%d [label=\"%s\"];\n", node->id, nodeName);
  for(int i = 0; i < node->nChildren; i++) {
    printf("%d -> %d;\n", node->id, node->children[i]->id);
    graphvizAstRec(node->children[i]);
  }
}

