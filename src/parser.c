#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

#define DEBUG

void shift();
int reduce();
void printStack();

// Replaces the current stack top with a parent node of specified type
void singleParent(NodeType type);

// Checks whether a certain type can come before a statement
int canPrecedeStatement(Node* node);

void parserStart() {
  nextToken = 0;
  initializeStack();

  while(nextToken < lexerState.nTokens) {
    shift();
    int success = 0;

    do {
      success = reduce(); 

#ifdef DEBUG
    printf("After reduce. ");
    printStack();
#endif

    } while(success);
  }
}

void shift() {
  Token* token = &lexerState.tokens[nextToken];
  nextToken++;

  Node node = { NTTerminal, token, NULL};
  createAndPush(node, 0);

#ifdef DEBUG
    printf("After shift.  ");
    printStack();
#endif

}

int reduce() {
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];

  int reduced = 0;

#ifdef DEBUG
    printf("Entering reduce with type %d\n", curNode->type);
#endif


  if(curNode->type == NTTerminal && curNode->token->type == TTSemi) {

#ifdef DEBUG
    printf("is semi\n");
#endif

    if(!prevNode || prevNode->type == NTProgramPart) {
      // we are reading a semicolon and the previous statement is already
      // reduced.
      singleParent(NTNoop);
      reduced = 1;
    } else if(prevNode->type == NTTerminal 
              && prevNode->token->type == TTBreak) {
      stackPop(2);
      Node node = { NTBreakSt, NULL, NULL };
      Node* nodePtr = createAndPush(node, 2);
      nodePtr->children[0] = prevNode;
      nodePtr->children[1] = curNode;
      reduced = 1;
    } else if(prevNode->type == NTTerminal 
              && prevNode->token->type == TTNext) {
      stackPop(2);
      Node node = { NTNextSt, NULL, NULL };
      Node* nodePtr = createAndPush(node, 2);
      nodePtr->children[0] = prevNode;
      nodePtr->children[1] = curNode;
      reduced = 1;
    }

  } else if(curNode->type == NTTerminal && isLiteral(curNode->token->type)) {

#ifdef DEBUG
    printf("is literal\n");
#endif

    // we have a literal, becomes term
    singleParent(NTTerm);
    reduced = 1;

  } else if(curNode->type == NTStatement) {

#ifdef DEBUG
    printf("is statement\n");
#endif

    if(!prevNode || prevNode->type == NTProgramPart) {
      // we have a finished statement and the previous statement is already
      // reduced.
      singleParent(NTProgramPart);
      reduced = 1;
    } else if(!canPrecedeStatement(prevNode)) {
      char* format = "Unexpected NT %d before statement.";
      int len = strlen(format) + 6;
      char str[len];
      sprintf(str, format, prevNode->type); 
      error(str);
    }

  } else if(isSubStatement(curNode->type)) {

#ifdef DEBUG
    printf("is sub statement\n");
#endif

    singleParent(NTStatement);
    reduced = 1;

  }

  return reduced;

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
  printf("Stack: ");
  for(int i = 0; i <= pStack.pointer; i++) {
    Node* node = pStack.nodes[i];
    printf(" %d", node->type);
  }
  printf(".\n");
}

int canPrecedeStatement(Node* node) {
  NodeType type = node->type;

  if(type == NTProgramPart || type == NTStatement) return 1;
  if(type == NTTerminal) {
    TokenType ttype = node->token->type;
    if(ttype == TTColon || ttype == TTComma || ttype == TTElse ||
       ttype == TTArrow || ttype == TTLBrace)
      return 1;
  }
  return 0;
}

