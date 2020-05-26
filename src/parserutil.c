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

Node* createAndPush(Node node, int nChildren) {
  Node* nodePtr = newNode(node);
  if(nChildren > 0) allocChildren(nodePtr, nChildren);
  stackPush(nodePtr);
  return nodePtr;
}

Node* newNode(Node node) {
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
  if(parserState.nextToken < lexerState.nTokens)
    return lexerState.tokens[parserState.nextToken];

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
     type == NTMatchSt || type == NTCallSt || type == NTAssignment)
    return 1;
  return 0;
}

void parsError(char* msg, int lnum, int chnum) {
  if(lnum > 0) {
    printf("\nERROR: %s\n%s: line: %d, column: %d.\n", 
      msg, parserState.filename, lnum, chnum);
    printCharInFile(parserState.file, lnum, chnum);
  } else {
    printf("\nERROR: %s\n%s.\n", msg, parserState.filename);
  }
  exit(1);
}

void printStack() {
  printf("Stack: ");
  for(int i = 0; i <= pStack.pointer; i++) {
    Node* node = pStack.nodes[i];
    printf(" %d", node->type);
  }
  printf(".\n");
}

void strReplaceNodeName(char* str, char* format, Node* node) {
  NodeType type = node->type;

  if(type == NTTerminal) {
    char strToken[node->token->nameSize + 10];
    char* strTokenFormat = "token '%s'";
    sprintf(strToken, strTokenFormat, node->token->name);
    sprintf(str, format, strToken);
  } 
  else switch(type) {
    case NTBreakSt: sprintf(str, format, "'break' statement");
      break;
    case NTNextSt: sprintf(str, format, "'next' statement");
      break;
    case NTIfSt: sprintf(str, format, "'if' statement");
      break;
    case NTLoopSt: sprintf(str, format, "'loop' statement");
      break;
    case NTWhileSt: sprintf(str, format, "'while' statement");
      break;
    case NTNoop: sprintf(str, format, "empty statement");
      break;
    case NTMatchSt: sprintf(str, format, "'match' statement");
      break;
    case NTProgramPart: sprintf(str, format, 
                                "function declaration or statement");
      break;
    case NTStatement: sprintf(str, format, "statement");
      break;
    case NTFunction: sprintf(str, format, "function declaration");
      break;
    case NTExpression: sprintf(str, format, "expression");
      break;
    case NTTerm: sprintf(str, format, "term");
      break;
    case NTLiteral: sprintf(str, format, "literal");
      break;
    case NTBinaryOp: sprintf(str, format, "binary operator");
      break;
    case NTProgram: sprintf(str, format, "complete program");
      break;
    case NTType: sprintf(str, format, "type");
      break;
    case NTIdentifier: sprintf(str, format, "identifier");
      break;
    case NTDeclaration: sprintf(str, format, "declaration");
      break;
    default: sprintf(str, format, "NT");
      break;
  }
}

void strReplaceNodeAbbrev(char* str, char* format, Node* node) {
  NodeType type = node->type;

  if(type == NTTerminal) {
    char strToken[node->token->nameSize + 10];
    char* strTokenFormat = "%s";
    sprintf(strToken, strTokenFormat, node->token->name);
    sprintf(str, format, strToken);
  } 
  else switch(type) {
    case NTBreakSt: sprintf(str, format, "BREAK st");
      break;
    case NTNextSt: sprintf(str, format, "NEXT st");
      break;
    case NTIfSt: sprintf(str, format, "IF st");
      break;
    case NTLoopSt: sprintf(str, format, "LOOP st");
      break;
    case NTWhileSt: sprintf(str, format, "WHILE st");
      break;
    case NTNoop: sprintf(str, format, "NOOP");
      break;
    case NTMatchSt: sprintf(str, format, "MATCH st");
      break;
    case NTProgramPart: sprintf(str, format, 
                                "PP");
      break;
    case NTStatement: sprintf(str, format, "STAT");
      break;
    case NTFunction: sprintf(str, format, "F DECL");
      break;
    case NTExpression: sprintf(str, format, "EXPR");
      break;
    case NTTerm: sprintf(str, format, "TERM");
      break;
    case NTLiteral: sprintf(str, format, "LIT");
      break;
    case NTBinaryOp: sprintf(str, format, "OP");
      break;
    case NTProgram: sprintf(str, format, "PROGRAM");
      break;
    case NTType: sprintf(str, format, "TYPE");
      break;
    case NTIdentifier: sprintf(str, format, "ID");
      break;
    case NTDeclaration: sprintf(str, format, "DECL");
      break;
    default: sprintf(str, format, "NT");
      break;
  }
}

