/*
 *
 *
 * The AST is built by the parser, but this code file is meant to be
 * responsible to navigate and manipulate the AST after (but also during)
 * the parsing.
 *
 */

#include "parser.h"

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
