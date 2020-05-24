#include <stdlib.h>
#include "parser.h"

#define INITIAL_STACK_SIZE 100

void initializeStack() {
  pStack = (ParserStack) {
    .pointer = -1,
    .nodes = NULL,
    .maxSize = INITIAL_STACK_SIZE
  };

  pStack.nodes = (Node*) malloc(INITIAL_STACK_SIZE * sizeof(Node));
}

void stackPush(Node node) {
  if(pStack.pointer >= pStack.maxSize - 1) { // reallocate space for stack
    pStack.nodes = (Node*) realloc(
      pStack.nodes, sizeof(Node) * pStack.maxSize * 2);
  }

  pStack.pointer++;
  pStack.nodes[pStack.pointer] = node;
}

Node stackPop(int n) {
  Node node = pStack.nodes[pStack.pointer - (n - 1)];
  pStack.pointer -= n;
  return node;
}

Token lookAhead() {
  if(nextToken < lexerState.nTokens)
    return lexerState.tokens[nextToken];

  // Ideally this function shouldn't be called if there are no tokens left.
  Token token = { NULL, 0, TTEof, 0, 0 };
  return token;
}

void allocChildren(Node* node, int nChildren) {
  node->children = (Node*) malloc(sizeof(Node) * nChildren);
}
