/*
 *
 * Helper functions for the parser. Mostly data structures.
 *
 * The parser will use a stack of pointers to subtrees. The nodes of these
 * subtress will be in a list.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "parser.h"

#define INITIAL_STACK_SIZE 100
#define INITIAL_MAX_NODES 250

void initializeStack() {
  pStack = (ParserStack) {
    .pointer = -1,
    .nodes = NULL,
    .maxSize = INITIAL_STACK_SIZE
  };

  nodeCount = 0;
  maxNodes = INITIAL_MAX_NODES;
  pNodes = (Node*) malloc(sizeof(Node) * maxNodes);
  pStack.nodes = (Node**) malloc(INITIAL_STACK_SIZE * sizeof(Node*));
}

Node* createAndPush(NodeType type, int nChildren) {
  Node* nodePtr = newNode(type);
  if(nChildren > 0) allocChildren(nodePtr, nChildren);
  stackPush(nodePtr);
  return nodePtr;
}

Node* newNode(NodeType type) {
  Node node = { 
    .type = type, 
    .token = NULL, 
    .children = NULL
  };

  if(nodeCount >= maxNodes) {
    pNodes = (Node*) realloc(pNodes, sizeof(Node) * maxNodes * 2);
    maxNodes *= 2;
  }

  pNodes[nodeCount] = node;
  pNodes[nodeCount].id = nodeCount;
  nodeCount++;
  return &pNodes[nodeCount - 1];
}

void stackPush(Node* node) {
  if(pStack.pointer >= pStack.maxSize - 1) { // reallocate space for stack
    pStack.nodes = (Node**) realloc(
      pStack.nodes, sizeof(Node*) * pStack.maxSize * 2);

    pStack.maxSize *= 2;
  }
  pStack.pointer++;
  pStack.nodes[pStack.pointer] = node;
}

Node* stackPop(int n) {
  Node* node = pStack.nodes[pStack.pointer - (n - 1)];
  pStack.pointer -= n;
  return node;
}

Token lookAhead() {
  if(parserState.nextToken < parserState.nTokens)
    return parserState.tokens[parserState.nextToken];

  // Ideally this function shouldn't be called if there are no tokens left.
  Token token = { NULL, 0, TTEof, 0, 0 };
  return token;
}

void allocChildren(Node* node, int nChildren) {
  node->children = (Node**) malloc(sizeof(Node*) * nChildren);
  node->nChildren = nChildren;
}

int isSubStatement(NodeType type) {
  if(type == NTBreakSt || type == NTNextSt || type == NTIfSt ||
     type == NTLoopSt || type == NTWhileSt || type == NTNoop ||
     type == NTMatchSt || type == NTAssignment)
    return 1;
  return 0;
}

void parsErrorHelper(char* format, Node* node, Node* leafNode) {
  int len = strlen(format) + MAX_NODE_NAME;
  char str[len];
  strReplaceNodeName(str, format, node); 
  parsError(str, leafNode->token->lnum, leafNode->token->chnum);
}

void parsError(char* msg, int lnum, int chnum) {
  if(lnum > 0) {
    printf("\nSyntax " ERROR_COLOR_START "ERROR" COLOR_END ": %s\n", msg);
    printCharInFile(parserState.file, parserState.filename, lnum, chnum);
  } else {
    printf("\nSyntax " ERROR_COLOR_START "ERROR" COLOR_END 
      ": %s\n%s.\n", msg, parserState.filename);
  }
  exit(1);
}

void printStack() {
  printf("Stack: ");
  for(int i = 0; i <= pStack.pointer; i++) {
    Node* node = pStack.nodes[i];

    if(node->type == NTTerminal)
      printf(" %s", node->token->name);
    else
      printf(" %d", node->type);
  }
  printf(".\n");
}

