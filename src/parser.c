#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

#define DEBUG

void shift();
int reduce();
int reduceSemi();
int reduceExpression();
int reduceRPar();
int reduceFunctionCall();
int reduceIdentifier();
void reduceRoot();

// Replaces the current stack top with a parent node of specified type
void singleParent(NodeType type);

// Checks whether a certain type can come before a statement
int canPrecedeStatement(Node* node);

void parserStart(FILE* file, char* filename, int nTokens, Token* tokens) {
  parserState = (ParserState) { 
    .file = file, 
    .filename = filename,
    .nextToken = 0,
    .nTokens = nTokens,
    .tokens = tokens
  };

  initializeStack();

  while(parserState.nextToken < parserState.nTokens) {
    shift();
    int success = 0;

    do {
      success = reduce(); 

#ifdef DEBUG
//printStack();  
#endif

    } while(success);
  }

#ifdef DEBUG
  graphvizAst(pStack.nodes[pStack.pointer]);
#endif
}

void shift() {
  Token* token = &parserState.tokens[parserState.nextToken];
  parserState.nextToken++;

  Node node = { 
    .type = NTTerminal, 
    .token = token, 
    .children = NULL,
    .nChildren = 0
  };
  createAndPush(node, 0);
}

int reduce() {
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];

  int reduced = 0;

  if(curNode->type == NTProgramPart) { // DONE
    if(lookAhead().type == TTEof) {
      reduceRoot();
      reduced = 1;
    }
  } else if(curNode->type == NTStatement) { // UNFINISHED

    if(!prevNode || prevNode->type == NTProgramPart) {
      // we have a finished statement and the previous statement is already
      // reduced.
      singleParent(NTProgramPart);
      reduced = 1;
    } else if(!canPrecedeStatement(prevNode)) {
      // build error string
      parsErrorHelper("Unexpected %s before statement.",
        prevNode, astLastLeaf(prevNode));
    } else if(prevNode->type == NTTerminal &&
              prevNode->token->type == TTColon) {
      Node* ifNode = NULL;
      if(pStack.pointer >= 3) ifNode = pStack.nodes[pStack.pointer - 3];

      if(ifNode->type == NTTerminal && ifNode->token->type == TTIf) {
        Node* condNode = pStack.nodes[pStack.pointer - 2];

        if(condNode->type != NTExpression) { // error
          parsErrorHelper("Unexpected %s as condition for 'if' statement.",
            condNode, astFirstLeaf(condNode));
        }

        // if statement
        if(lookAhead().type == TTElse) { 
          // with else clause -- do nothing now
        } else { // without else
          stackPop(4);
          Node node = { 
            .type = NTIfSt, 
            .token = NULL, 
            .children = NULL // set in createAndPush()
          };
          Node* nodePtr = createAndPush(node, 2);
          nodePtr->children[0] = condNode;
          nodePtr->children[1] = curNode;
          reduced = 1;
        }
      }
    } else if(prevNode->type == NTTerminal && prevNode->token->type == TTElse) {
      if(pStack.pointer < 5) { // error: incomplete if statement
        Node* problematic = astFirstLeaf(prevNode);
        parsError("Malformed 'if' statement.", problematic->token->lnum, 
          problematic->token->chnum);
      }

      Node* thenNode = pStack.nodes[pStack.pointer - 2];
      Node* condNode = pStack.nodes[pStack.pointer - 4];
      Node* ifNode = pStack.nodes[pStack.pointer - 5];

      if(thenNode->type != NTStatement) { // error: statement expected
        parsErrorHelper("Bad 'if' statement: expected statement, found %s.",
          thenNode, astFirstLeaf(thenNode));
      }
      if(condNode->type != NTExpression) { // error: expression expected
        parsErrorHelper("Bad 'if' condition: expected expression, found %s.",
          condNode, astFirstLeaf(condNode));
      }
      if(ifNode->type != NTTerminal || ifNode->token->type != TTIf) { 
        // error: 'if' keyword expected
        parsErrorHelper("Bad 'if' statement: expected 'if' keyword, found %s.",
          ifNode, astFirstLeaf(ifNode));
      }

      stackPop(6);
      Node node = { 
        .type = NTIfSt, 
        .token = NULL, 
        .children = NULL // set in createAndPush()
      };
      Node* nodePtr = createAndPush(node, 3);
      nodePtr->children[0] = condNode;
      nodePtr->children[1] = thenNode;
      nodePtr->children[2] = curNode;
      reduced = 1;
    }

  } else if(curNode->type == NTFunction) { // DONE
    singleParent(NTProgramPart);
    reduced = 1;
  } else if(curNode->type == NTDeclaration) { // DONE
    if(!prevNode || prevNode->type == NTStatement
       || prevNode->type == NTProgramPart) { // independent declaration
      singleParent(NTProgramPart);
      reduced = 1;
    } else { // declaration part of FOR statement -- do nothing
    }
  } else if(isSubStatement(curNode->type)) { // DONE
    // Substatements: NTIfSt, NTNoop, NTNextSt, NTBreakSt, NTWhileSt,
    // NTMatchSt, NTLoopSt. 
    singleParent(NTStatement);
    reduced = 1;
  } else if(curNode->type == NTExpression) { // UNFINISHED 
    reduced = reduceExpression();
  } else if(curNode->type == NTCallExpr) { // DONE
    if(!prevNode || prevNode->type == NTStatement
       || prevNode->type == NTProgramPart) { // call statement 
      singleParent(NTStatement);
      reduced = 1;
    } else { // call expression 
      singleParent(NTExpression);
      reduced = 1;
    }
  } else if(curNode->type == NTTerm) { // UNFINISHED
    singleParent(NTExpression);
    reduced = 1;
  } else if(curNode->type == NTLiteral) { // DONE
    //singleParent(NTTerm);
    singleParent(NTExpression);
    reduced = 1;
  } else if(curNode->type == NTBinaryOp) { // UNFINISHED
  } else if(curNode->type == NTIdentifier) { // UNFINISHED
    reduced = reduceIdentifier();
  } else if(curNode->type == NTTerminal) { // UNFINISHED
    TokenType ttype = curNode->token->type;

    if(isBinaryOp(ttype)) {
      singleParent(NTBinaryOp);
      reduced = 1;
    } else if(isLiteral(ttype)) {
      singleParent(NTLiteral);
      reduced = 1;
    } else if(isType(ttype)) {
      singleParent(NTType);
      reduced = 1;
    } else if(ttype == TTId) {
      singleParent(NTIdentifier);
      reduced = 1;
    } else if(ttype == TTBreak) { // wait next shift
    } else if(ttype == TTNext) { // wait next shift
    } else if(ttype == TTRPar) {
      reduced = reduceRPar();
    } else if(ttype == TTSemi) {
      reduced = reduceSemi();
    }
  }

  return reduced;
}

int reduceRPar() {
  int reduced = 0;
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  Node* prevPrevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];
  if(pStack.pointer >= 2) prevPrevNode = pStack.nodes[pStack.pointer - 2];

  if(!prevNode) {
    Node* problematic = astLastLeaf(curNode);
    parsError("Program beginning with ')'.", 
      problematic->token->lnum, problematic->token->chnum);
  }
  if(prevNode->type == NTProgramPart || prevNode->type == NTStatement) {
    parsErrorHelper("Unexpected ')' after %s.",
      prevNode, astLastLeaf(prevNode));
  }

  if(prevNode->type == NTExpression) {
    if(prevPrevNode && prevPrevNode->type == NTTerminal &&
       prevPrevNode->token->type == TTLPar) {
      // (EXPR) -- reduce
      stackPop(3);
      Node node = { 
        .type = NTExpression, 
        .token = NULL, 
        .children = NULL
      };
      Node* nodePtr = createAndPush(node, 1);
      nodePtr->children[0] = prevNode; // parentheses are ignored
      reduced = 1;
    }
  } else if(prevNode->type == NTCallParam ||
            (prevNode->type == NTTerminal && prevNode->token->type == TTLPar)) {
    reduced = reduceFunctionCall();
  }

  return reduced;
}

int reduceFunctionCall() {
  int reduced = 0;
  Node* prevNode = NULL;
  prevNode = pStack.nodes[pStack.pointer - 1];

  if(prevNode->type == NTCallParam) { // at least one param
    Node* idNode = prevNode;
    int nParams = 0;
    int idIndex = 0;

    while(idNode->type != NTIdentifier) { // find the function name
      nParams++;
      if(pStack.pointer >= 1 + (nParams * 2)) {
        idIndex = pStack.pointer - (1 + nParams * 2);
        idNode = pStack.nodes[idIndex];
      } else { // this should never happen (as param checks for this)
        Node* problematic = astFirstLeaf(prevNode);
        parsError("Malformed function call expression.", 
          problematic->token->lnum, problematic->token->chnum);
      }
    }

    Node node = { 
      .type = NTCallExpr, 
      .token = NULL, 
      .children = NULL
    };
    Node* nodePtr = newNode(node);
    allocChildren(nodePtr, nParams + 1);
    nodePtr->children[0] = idNode;

    for(int i = 1; i <= nParams; i++)
      nodePtr->children[i] = pStack.nodes[idIndex + i * 2];

    stackPop(2 + nParams * 2);
    stackPush(nodePtr);
    reduced = 1;
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTLPar) {
    // ID()  -- function call without params
    Node* idNode = pStack.nodes[pStack.pointer - 2];

    stackPop(3);
    Node node = { 
      .type = NTCallExpr, 
      .token = NULL, 
      .children = NULL
    };
    Node* nodePtr = createAndPush(node, 1);
    nodePtr->children[0] = idNode; // parentheses are ignored
    reduced = 1;
  }

  return reduced;
}

int reduceIdentifier() {
  int reduced = 0;
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];
  TokenType ttype = lookAhead().type;

  if(ttype == TTId || ttype == TTLPar || ttype == TTNot || isLiteral(ttype)) {
    // is function ID (will be reduced later)
  } else { // is variable
    if(prevNode && prevNode->type == NTType) // in declaration
    {
      Node* prevPrevNode = NULL;
      if(pStack.pointer >= 2) prevPrevNode = pStack.nodes[pStack.pointer - 2];

      if(!prevPrevNode || prevPrevNode->type == NTProgramPart ||
         prevPrevNode->type == NTStatement)
      {
        // variable declaration, will reduce later
      } else { // is parameter
        stackPop(2);
        Node node = { 
          .type = NTParam, 
          .token = NULL, 
          .children = NULL 
        };
        Node* nodePtr = createAndPush(node, 2);
        nodePtr->children[0] = prevNode;
        nodePtr->children[1] = curNode;
        reduced = 1;
      }
    } else { // term
      //singleParent(NTTerm);
      singleParent(NTExpression);
      reduced = 1;
    }
  }

  return reduced;
}

int reduceExpression() {
  int reduced = 0;
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  Node* prevPrevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];
  if(pStack.pointer >= 2) prevPrevNode = pStack.nodes[pStack.pointer - 2];

  Token laToken = lookAhead();
  TokenType laType = laToken.type;

  if(!prevNode) { // error: starting program with expression
    Node* problematic = astLastLeaf(curNode);
    parsError("Program beginning with expression.", 
      problematic->token->lnum, problematic->token->chnum);
  }
  else if(prevNode->type == NTProgramPart || prevNode->type == NTStatement) {
    // Error: expression after complete statement
    parsErrorHelper("Unexpected expression after %s.",
      prevNode, astLastLeaf(prevNode));
  }

  if(prevNode->type == NTTerminal && prevNode->token->type == TTLPar) {
    if(!prevPrevNode) { // error: beginning program with parenthesis
      // TODO: make a check for illegal beginnings so we don't have to check
      // this all the time
      Node* problematic = astFirstLeaf(prevNode);
      parsError("Program beginning with parenthesis.", 
        problematic->token->lnum, problematic->token->chnum);
    } else if(prevPrevNode->type == NTIdentifier) { 
      if(laType == TTComma || laType == TTRPar) {
        // ID ( EXPR ,   or   ID ( EXPR )   -- call parameter
        singleParent(NTCallParam);
        reduced = 1;
      }
    }
  }
  else if(prevNode->type == NTTerminal && prevNode->token->type == TTComma) {
    if(!prevPrevNode) { // error: beginning program with parenthesis
      // TODO: make a check for illegal beginnings so we don't have to check
      // this all the time
      Node* problematic = astFirstLeaf(prevNode);
      parsError("Program beginning with a comma.", 
        problematic->token->lnum, problematic->token->chnum);
    } else if(prevPrevNode->type == NTCallParam) { // another call param
      singleParent(NTCallParam);
      reduced = 1;
    } else if(prevPrevNode->type == NTDeclaration) { // for statement
      // do nothing
    } else { // unexpected node before: , EXPR
      parsErrorHelper("Expected parameter or declaration, found %s.",
        prevPrevNode, astFirstLeaf(prevPrevNode));
    }
  }
  else if(isExprTerminator(laType)) {
    if(prevNode->type == NTBinaryOp) {
      if(prevPrevNode && prevPrevNode->type != NTExpression) {
        // If we see a OP EXPR sequence, there must be an EXPR before that 
        parsErrorHelper("Expected expression before operator, found %s.",
          prevPrevNode, astLastLeaf(prevPrevNode));
      } else { // EXPR OP EXPR TERMINATOR sequence, EXPR OP EXPR => EXPR
        stackPop(3);
        Node node = { 
          .type = NTExpression, 
          .token = NULL, 
          .children = NULL // set in createAndPush()
        };
        Node* nodePtr = createAndPush(node, 3);
        nodePtr->children[0] = prevPrevNode;
        nodePtr->children[1] = prevNode;
        nodePtr->children[2] = curNode;
        reduced = 1;
      }
    }
  } else if(isBinaryOp(laType)) {
    if(prevNode->type == NTBinaryOp) {
      if(prevPrevNode && prevPrevNode->type != NTExpression) {
        // If we see a OP EXPR sequence, there must be an EXPR before that 
        parsErrorHelper("Expected expression before operator, found %s.",
          prevPrevNode, astLastLeaf(prevPrevNode));
      } else if(precedence(laType) >= 
                precedence(prevNode->children[0]->token->type)){ 
        // EXPR OP EXPR OP sequence, reduce if the previous has precedence
        // (<= value for precedence()), otherwise do nothing
        stackPop(3);
        Node node = { 
          .type = NTExpression, 
          .token = NULL, 
          .children = NULL // set in createAndPush()
        };
        Node* nodePtr = createAndPush(node, 3);
        nodePtr->children[0] = prevPrevNode;
        nodePtr->children[1] = prevNode;
        nodePtr->children[2] = curNode;
        reduced = 1;
      }
    }
  } else {
    // error: unexpected token after expression
    char* format = "Unexpected token '%s' after expression.";
    char str[strlen(format) + laToken.nameSize + 5];
    sprintf(str, format, laToken.name);
    parsError(str, laToken.lnum, laToken.chnum);
  }

  return reduced;
}

int reduceSemi() {
  int reduced = 0;
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  Node* prevPrevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];
  if(pStack.pointer >= 2) prevPrevNode = pStack.nodes[pStack.pointer - 2];

  if(!prevNode || prevNode->type == NTProgramPart) {
    singleParent(NTNoop);
    reduced = 1;
  } else if(prevNode->type == NTTerminal) {
    TokenType ttype = prevNode->token->type;

    if(ttype == TTBreak || ttype == TTNext) {
      NodeType nType = NTBreakSt;
      if(ttype == TTNext) nType = NTNextSt;

      stackPop(2);
      Node node = { 
        .type = nType, 
        .token = NULL, 
        .children = NULL 
      };
      Node* nodePtr = createAndPush(node, 2);
      nodePtr->children[0] = prevNode;
      nodePtr->children[1] = curNode;
      reduced = 1;
    }
  } else if(prevNode->type == NTExpression) {
    if(prevPrevNode && prevPrevNode->type == NTTerminal &&
       prevPrevNode->token->type == TTAssign) { 
      // assignment or declaration with assignment
      Node* prev3 = NULL;
      Node* prev4 = NULL;
      if(pStack.pointer >= 1) prev3 = pStack.nodes[pStack.pointer - 3];
      if(pStack.pointer >= 2) prev4 = pStack.nodes[pStack.pointer - 4];

      if(prev3 && prev3->type == NTIdentifier) {
        if(!prev4) { // error
          char* format = "Assignment to undeclared variable '%s'.";
          int len = strlen(format) + prev3->children[0]->token->nameSize;
          char str[len];
          sprintf(str, format, prev3->children[0]->token->name);

          Node* problematic = astFirstLeaf(prev3);
          parsError(str, problematic->token->lnum, problematic->token->chnum);
        } else if(prev4->type == NTProgramPart || prev4->type == NTStatement) {
          // ID = EXPR ;  -- assignment
          stackPop(4);
          Node node = { 
            .type = NTAssignment, 
            .token = NULL, 
            .children = NULL // set in createAndPush()
          };
          Node* nodePtr = createAndPush(node, 2);
          nodePtr->children[0] = prev3; // ID
          nodePtr->children[1] = prevNode; // EXPR
          reduced = 1;
        } else if(prev4->type == NTType) { // declaration w/ assignment
          // TYPE ID = EXPR ;  -- declaration with assignment
          stackPop(5);
          Node node = { 
            .type = NTDeclaration, 
            .token = NULL, 
            .children = NULL // set in createAndPush()
          };
          Node* nodePtr = createAndPush(node, 3);
          nodePtr->children[0] = prev4; // TYPE
          nodePtr->children[1] = prev3; // ID
          nodePtr->children[2] = prevNode; // EXPR
          reduced = 1;
        }
      } else { // error
        parsErrorHelper("Assignment to %s.",
          prev3, astFirstLeaf(prev3));
      }
    }
  }

  return reduced;
}

void reduceRoot() {
  for(int i = 0; i <= pStack.pointer; i++) {
    if(pStack.nodes[i]->type != NTProgramPart) {
      // build error string
      parsErrorHelper("Unexpected %s at program root level.",
        pStack.nodes[i], astFirstLeaf(pStack.nodes[i]));
    }
  }

  Node node = { 
    .type = NTProgram, 
    .token = NULL, 
    .children = NULL 
  };
  Node* nodePtr = newNode(node);
  allocChildren(nodePtr, pStack.pointer + 1);

  for(int i = 0; i <= pStack.pointer; i++) {
    nodePtr->children[i] = pStack.nodes[i];
  }

  stackPop(pStack.pointer + 1);
  stackPush(nodePtr);
}

void singleParent(NodeType type) {
  Node* curNode = pStack.nodes[pStack.pointer];
  stackPop(1);

  Node node = { 
    .type = type, 
    .token = NULL, 
    .children = NULL 
  };
  Node* nodePtr = createAndPush(node, 1);
  nodePtr->children[0] = curNode;
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

