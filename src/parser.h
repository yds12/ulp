/*
 * Header file for the parser and parser related functions, structs, enums
 * and global variables.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "datast.h"

typedef struct stParserStack {
  Node** nodes;
  int pointer;
  int maxSize;
} ParserStack;

typedef struct stParserState {
  FILE* file;
  char* filename;
  int nextToken;
  int nTokens;  // from lexerState
  Token* tokens;  // from lexerState
  int maxNodes;  // current size of the list of nodes
  int nodeCount;  // number of nodes created so far
} ParserState;

// Global state of the parser
ParserState parserState;

// The stack of subtrees of the LR parser
ParserStack pStack;

// The list of nodes that comprise the trees
Node* pNodes;

void parserStart();

void initializeStack();

Node* newNode(NodeType type);

Node* createAndPush(NodeType type, int nChildren, ...);

void stackPush(Node* node);

Node* stackPop(int n);

void allocChildren(Node* node, int nChildren);

Token lookAhead();

Node* fromStackSafe(int offset);

int isSubStatement(NodeType type);

int isAssignmentOp(TokenType type);

void assertTokenEqual(Node* node, TokenType ttype, char* msg);

void assertEqual(Node* node, NodeType type, char* msg);

void parsErrorHelper(char* format, Node* node, Node* leafNode);

void parsError(char* msg, int lnum, int chnum);

void printStack();

#endif

