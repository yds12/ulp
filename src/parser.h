/*
 * Header file for the parser and parser related functions, structs, enums
 * and global variables.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum stNodeType {
  NTProgram,
  NTProgramPart,
  NTStatement,
  NTFunction,
  NTBreakSt,
  NTNextSt,
  NTIfSt,
  NTLoopSt,
  NTWhileSt,
  NTMatchSt,
  NTNoop,
  NTExpression,
  NTTerm,
  NTLiteral,
  NTBinaryOp,
  NTTerminal,
} NodeType;

typedef struct stNode {
  NodeType type;
  Token* token;
  struct stNode** children;
} Node;

typedef struct stParserStack {
  Node** nodes;
  int pointer;
  int maxSize;
} ParserStack;

// The stack of subtrees of the LR parser
ParserStack pStack;

// The list of nodes that comprise the trees
Node* pNodes;

// Current size of the list of nodes
int maxNodes;

// Number of nodes created so far
int nodeCount;

int nextToken;

void parserStart();

void initializeStack();

Node* newNode(Node node);

Node* createAndPush(Node node, int nChildren);

void stackPush(Node* node);

Node* stackPop(int n);

void allocChildren(Node* node, int nChildren);

Token lookAhead();

int isSubStatement(NodeType type);

#endif

