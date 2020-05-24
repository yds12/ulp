#include <stdio.h>
#include "lexer.h"
#include "parser.h"

#define DEBUG

void shift();
int reduce();
void printStack();

void parserStart() {
  nextToken = 0;
  initializeStack();

  while(nextToken < lexerState.nTokens) {
    shift();
    int success = 0;

    do {
      success = reduce(); 
    } while(success);

#ifdef DEBUG
    printStack();
#endif
  }
}

void shift() {

#ifdef DEBUG
      printf("shift\n");
#endif

  Token* token = &lexerState.tokens[nextToken];
  nextToken++;

  Node node = { NTTerminal, token, NULL};
  stackPush(node);
}

int reduce() {
  Node* curNode = &pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = &pStack.nodes[pStack.pointer - 1];

#ifdef DEBUG
      printf("Reduce. curNode->type: %d\n", curNode->type);

      if(curNode->token)
        printf("pStack.pointer: %d, curNode->token->type: %d\n", 
          pStack.pointer, curNode->token->type);
      if(!prevNode)
        printf("prevNode is NULL\n");
      else
        printf("prevNode->type:%d\n", prevNode->type);
#endif

  if(curNode->type == NTTerminal && curNode->token->type == TTSemi) {
    if(!prevNode || prevNode->type == NTProgramPart) {
      // we are reading a semicolon and the previous statement is already
      // reduced.
      Node node = { NTNoop, NULL, NULL };
      allocChildren(&node, 1);
      node.children[0] = *curNode;
      stackPop(1);
      stackPush(node);

#ifdef DEBUG
      printf("NTNoop\n");
#endif

      return 1;
    }
  } else if(curNode->type == NTNoop) {
    Node node = { NTStatement, NULL, NULL };
    allocChildren(&node, 1);
    node.children[0] = *curNode;
    stackPop(1);
    stackPush(node);

#ifdef DEBUG
    printf("NTStatement\n");
#endif

    return 1;
  } else if(curNode->type == NTStatement) {
    if(!prevNode || prevNode->type == NTProgramPart) {
      // we have a finished statement and the previous statement is already
      // reduced.
      Node node = { NTProgramPart, NULL, NULL };
      allocChildren(&node, 1);
      node.children[0] = *curNode;
      stackPop(1);
      stackPush(node);

#ifdef DEBUG
      printf("NTProgramPart\n");
#endif

      return 1;
    }
  }

  return 0;

  /*Node* curNode = &pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = &pStack.nodes[pStack.pointer - 1];

  if(curNode->type == NTLiteral) {

    Node node = { NULL, NTTerm, NULL }; 
    allocChildren(&node, 1);
    node.children[0] = curNode;
    stackPop(1);
    stackPush(&node);

  } else if(curNode->type == NTTerm) {

    if(prevNode && prevNode->type == NTBinaryOp) {
      Node* prevPrevNode = NULL;
      if(pStack.pointer >= 2) prevPrevNode = &pStack.nodes[pStack.pointer - 2];
      else error("Missing operand."); // TODO: error should not use lexerState

      Node node = { NULL, NTTerm, NULL };
      allocChildren(&node, 3);
      node.children[0] = prevPrevNode;
      node.children[1] = prevNode;
      node.children[2] = curNode;
      stackPop(3);
      stackPush(&node);
    }
    else {
    }

  }*/
}

void printStack() {
  printf("Nodes on stack: ");
  for(int i = 0; i < pStack.pointer; i++) {
    Node node = pStack.nodes[pStack.pointer];
    printf(" %d", node.type);
  }
  printf(".\n");
}

