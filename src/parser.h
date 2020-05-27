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
} ParserState;

// Global state of the parser
ParserState parserState;

// The stack of subtrees of the LR parser
ParserStack pStack;

// The list of nodes that comprise the trees
Node* pNodes;

// Current size of the list of nodes
int maxNodes;

// Number of nodes created so far
int nodeCount;

void parserStart();

void initializeStack();

Node* newNode(Node node);

Node* createAndPush(Node node, int nChildren);

void stackPush(Node* node);

Node* stackPop(int n);

void allocChildren(Node* node, int nChildren);

Token lookAhead();

int isSubStatement(NodeType type);

void parsError(char* msg, int lnum, int chnum);

void printStack();

#endif

