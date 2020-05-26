/*
 * Header file for the parser and parser related functions, structs, enums
 * and global variables.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// A node type name cannot exceed this length
#define MAX_NODE_NAME 40

typedef enum stNodeType {
  NTProgram,
  NTProgramPart,
  NTStatement,
  NTFunction,
  NTDeclaration,
  NTBreakSt,
  NTNextSt,
  NTIfSt,
  NTLoopSt,
  NTWhileSt,
  NTMatchSt,
  NTCallSt,
  NTAssignment,
  NTCallParam,
  NTParam,
  NTNoop,
  NTExpression,
  NTTerm,
  NTType,
  NTLiteral,
  NTBinaryOp,
  NTIdentifier,
  NTTerminal,
} NodeType;

typedef struct stNode {
  NodeType type;
  Token* token;
  struct stNode** children;
  int nChildren;
  int id;
} Node;

typedef struct stParserStack {
  Node** nodes;
  int pointer;
  int maxSize;
} ParserStack;

typedef struct stParserState {
  FILE* file;
  char* filename;
  int nextToken;
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

void strReplaceNodeName(char* str, char* format, Node* node);

void strReplaceNodeAbbrev(char* str, char* format, Node* node);

void parsError(char* msg, int lnum, int chnum);

void printStack();

#endif

