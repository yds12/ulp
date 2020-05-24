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
  struct stNode* children;
} Node;

typedef struct stParserStack {
  Node* nodes;
  int pointer;
  int maxSize;
} ParserStack;

ParserStack pStack;

int nextToken;

void parserStart();

void initializeStack();

void stackPush(Node node);

Node stackPop(int n);

void allocChildren(Node* node, int nChildren);

Token lookAhead();

#endif

