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
#include <stdarg.h>
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

  parserState.nodeCount = 0;
  parserState.maxNodes = INITIAL_MAX_NODES;
  pNodes = (Node**) malloc(sizeof(Node*) * parserState.maxNodes);
  pStack.nodes = (Node**) malloc(INITIAL_STACK_SIZE * sizeof(Node*));
}

Node* createAndPush(NodeType type, int nChildren, ...) {
  Node* nodePtr = newNode(type);
  if(nChildren > 0) allocChildren(nodePtr, nChildren);
  stackPush(nodePtr);

  va_list nodePtrArgs;
  va_start(nodePtrArgs, nChildren);

  for(int i = 0; i < nChildren; i++) {
    nodePtr->children[i] = va_arg(nodePtrArgs, Node*);
  }
  va_end(nodePtrArgs);

  return nodePtr;
}

Node* newNode(NodeType type) {
  Node* node = (Node*) malloc(sizeof(Node));
  node->type = type;
  node->token = NULL;
  node->children = NULL;

  if(parserState.nodeCount >= parserState.maxNodes) {
    pNodes = (Node**) realloc(pNodes, sizeof(Node*) * parserState.maxNodes * 2);
    parserState.maxNodes *= 2;
  }

  pNodes[parserState.nodeCount] = node;
  pNodes[parserState.nodeCount]->id = parserState.nodeCount;
  parserState.nodeCount++;
  return pNodes[parserState.nodeCount - 1];
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
  Node* node = NULL;
  if(n >= 0) node = pStack.nodes[pStack.pointer - (n - 1)];

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
  
  for(int i = 0; i < nChildren; i++) node->children[i] = NULL;
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

void assertTokenEqual(Node* node, TokenType ttype, char* msg) {
  if(node->type != NTTerminal) {
    char* dfMsg = " Expected symbol, found %s.";
    char* finalMsg = (char*) malloc(
      sizeof(char) * (strlen(msg) + strlen(dfMsg) + MAX_NODE_NAME));
    char* format = (char*) malloc(
      sizeof(char) * (strlen(msg) + strlen(dfMsg)));

    strcpy(format, msg);
    strcat(format, dfMsg);
    strReplaceNodeName(finalMsg, format, node->type);

    Node* leafNode = astFirstLeaf(node);
    int lnum = leafNode->token->lnum;
    int chnum = leafNode->token->chnum;
    parsError(finalMsg, lnum, chnum);
  } else if(!(node->token)) {
    Node* problematic = astFirstLeaf(node);

    if(cli.outputType <= OUT_DEFAULT) {
      printf("Compiler bug: invalid terminal node.\n");
    }
    exit(1);
  } else if(node->token->type != ttype) {
    char* dfMsg = " Expected %s, found %s.";
    char* finalMsg = (char*) malloc(
      sizeof(char) * (strlen(msg) + strlen(dfMsg) + 2 * MAX_NODE_NAME));
    char* format = (char*) malloc(
      sizeof(char) * (strlen(msg) + strlen(dfMsg)));

    strcpy(format, msg);
    strcat(format, dfMsg);

    char* fmtName = "%s";
    char strWrong[MAX_NODE_NAME];
    char strExpect[MAX_NODE_NAME];
    strReplaceTokenName(strWrong, fmtName, node->token->type);
    strReplaceTokenName(strExpect, fmtName, ttype);
    sprintf(finalMsg, format, strExpect, strWrong);

    Node* leafNode = astFirstLeaf(node);
    int lnum = leafNode->token->lnum;
    int chnum = leafNode->token->chnum;
    parsError(finalMsg, lnum, chnum);
  }
}

void assertEqual(Node* node, NodeType type, char* msg) {
  if(node->type != type) {
    char* dfMsg = " Expected %s, found %s.";
    char* finalMsg = (char*) malloc(
      sizeof(char) * (strlen(msg) + strlen(dfMsg) + 2 * MAX_NODE_NAME));
    char* format = (char*) malloc(
      sizeof(char) * (strlen(msg) + strlen(dfMsg)));

    strcpy(format, msg);
    strcat(format, dfMsg);

    char* fmtName = "%s";
    char strWrong[MAX_NODE_NAME];
    char strExpect[MAX_NODE_NAME];
    strReplaceNodeAndTokenName(strWrong, fmtName, node);
    strReplaceNodeName(strExpect, fmtName, type);
    sprintf(finalMsg, format, strExpect, strWrong);

    Node* leafNode = astFirstLeaf(node);
    int lnum = leafNode->token->lnum;
    int chnum = leafNode->token->chnum;
    parsError(finalMsg, lnum, chnum);
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

