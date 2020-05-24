#include <stdio.h>
#include "lexer.h"
#include "parser.h"

#define DEBUG

void shift();
int reduce();
void printStack();

// Replaces the current stack top with a parent node of specified type
void singleParent(NodeType type);

void parserStart() {
  nextToken = 0;
  initializeStack();

  while(nextToken < lexerState.nTokens) {
    shift();

#ifdef DEBUG
    printf("After shift. ");
    printStack();
#endif

    int success = 0;

    do {
      success = reduce(); 

#ifdef DEBUG
    printf("After reduce. ");
    printStack();
#endif

    } while(success);
  }

  printf("Total tokens: %d\n", lexerState.nTokens);
}

void shift() {
  Token* token = &lexerState.tokens[nextToken];
  nextToken++;

  Node node = { NTTerminal, token, NULL};
  createAndPush(node, 0);
}

int reduce() {
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];

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
      singleParent(NTNoop);

#ifdef DEBUG
      printf("NTNoop\n");
#endif

      return 1;
    }
  } else if(curNode->type == NTTerminal && isLiteral(curNode->token->type)) {
    // we have a literal, becomes term
    singleParent(NTTerm);

#ifdef DEBUG
    printf("NTTerm\n");
#endif

    return 1;
  } else if(curNode->type == NTNoop) {
    singleParent(NTStatement);

#ifdef DEBUG
    printf("NTStatement\n");
#endif

    return 1;
  } else if(curNode->type == NTStatement) {
    if(!prevNode || prevNode->type == NTProgramPart) {
      // we have a finished statement and the previous statement is already
      // reduced.
      singleParent(NTProgramPart);

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

void singleParent(NodeType type) {
  Node* curNode = pStack.nodes[pStack.pointer];
  stackPop(1);

  Node node = { type, NULL, NULL };
  Node* nodePtr = createAndPush(node, 1);
  nodePtr->children[0] = curNode;
}

void printStack() {
  printf("Nodes on stack: ");
  for(int i = 0; i < pStack.pointer; i++) {
    Node* node = pStack.nodes[i];
    printf(" %d", node->type);
  }
  printf(".\n");
}

