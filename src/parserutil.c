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
#include "cli.h"
#include "util.h"
#include "parser.h"
#include "ast.h"

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

Node* fromStackSafe(int offset) {
  if(pStack.pointer >= offset) return pStack.nodes[pStack.pointer - offset];
  return NULL;
}

int isSubStatement(NodeType type) {
  if(type == NTBreakSt || type == NTNextSt || type == NTIfSt ||
     type == NTLoopSt || type == NTWhileSt || type == NTNoop ||
     type == NTMatchSt || type == NTAssignment || type == NTCallSt ||
     type == NTForSt || type == NTReturnSt)
    return 1;
  return 0;
}

int isAssignmentOp(TokenType type) {
  if(type == TTAssign || type == TTAdd || type == TTSub) return 1;
  return 0;
}

void assertTokenEqual(Node* node, TokenType ttype) {
  if(node->type != NTTerminal) {
    parsErrorHelper("Expected symbol, found %s.",
      node, astFirstLeaf(node));
  } else if(!(node->token)) {
    Node* problematic = astFirstLeaf(node);

    if(cli.outputType <= OUT_DEFAULT) {
      printf("Compiler bug: invalid terminal node.\n");
    }
    exit(1);
  } else if(node->token->type != ttype) {
    char* format = "%s";
    char strWrong[MAX_NODE_NAME];
    char strExpect[MAX_NODE_NAME];
    strReplaceTokenName(strWrong, format, node->token->type);
    strReplaceTokenName(strExpect, format, ttype);
    char* msgFormat = "Expected %s, found %s.";
    char finalString[strlen(msgFormat) + MAX_NODE_NAME * 2]; 
    sprintf(finalString, msgFormat, strExpect, strWrong);

    Node* problematic = astFirstLeaf(node);
    parsError(finalString, problematic->token->lnum, 
      problematic->token->chnum);
  }
}

void assertEqual(Node* node, NodeType type) {
  if(node->type != type) {
    char* format = "%s";
    char strWrong[MAX_NODE_NAME];
    char strExpect[MAX_NODE_NAME];
    strReplaceNodeAndTokenName(strWrong, format, node);
    strReplaceNodeName(strExpect, format, type);
    char* msgFormat = "Expected %s, found %s.";
    char finalString[strlen(msgFormat) + MAX_NODE_NAME * 2]; 
    sprintf(finalString, msgFormat, strExpect, strWrong);

    Node* problematic = astFirstLeaf(node);
    parsError(finalString, problematic->token->lnum, 
      problematic->token->chnum);
  }
}

void parsErrorHelper(char* format, Node* node, Node* leafNode) {
  int len = strlen(format) + MAX_NODE_NAME;
  char str[len];
  strReplaceNodeAndTokenName(str, format, node); 
  parsError(str, leafNode->token->lnum, leafNode->token->chnum);
}

void parsError(char* msg, int lnum, int chnum) {
  if(cli.outputType > OUT_DEFAULT) exit(1);

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
  if(cli.outputType > OUT_VERBOSE) return;

  printf("Stack: ");
  for(int i = 0; i <= pStack.pointer; i++) {
    Node* node = pStack.nodes[i];

    if(node->type == NTTerminal)
      printf(" %s", node->token->name);
    else
      printf(" %d", node->type);
  }
  printf("\n");
}

