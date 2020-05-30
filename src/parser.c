#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

#define DEBUG

void shift();
int reduce();
int reduceParam();
int reduceSemi();
int reduceExpression();
int reduceStatement();
int reduceRPar();
int reduceRBrace();
int reduceFunctionCallExpr();
int reduceFunctionCallSt();
int reduceIdentifier();
void reduceRoot();

// Replaces the current stack top with a parent node of specified type
void singleParent(NodeType type);

// Checks whether a certain type can come before a statement
int canPrecedeStatement(Node* node);

// Checks to see if all the tree is healthy
void checkTree(Node* node);

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
      printStack();  
      success = reduce(); 
    } while(success);
  }

  if(pStack.pointer > 0 || pStack.nodes[0]->type != NTProgram) {
    genericError("Failed to completely parse program.");
  }

  checkTree(fromStackSafe(0));
  graphvizAst(fromStackSafe(0));
}

void shift() {
  Token* token = &parserState.tokens[parserState.nextToken];
  parserState.nextToken++;
  Node* createdNode = createAndPush(NTTerminal, 0);
  createdNode->token = token;
}

int reduce() {
  Node* curNode = fromStackSafe(0);
  Node* prevNode = fromStackSafe(1);
  TokenType laType = lookAhead().type;

  int reduced = 0;

  if(curNode->type == NTProgramPart) {
    if(laType == TTEof) {
      reduceRoot();
      reduced = 1;
    }
  } else if(curNode->type == NTStatement) {
    reduced = reduceStatement();
  } else if(curNode->type == NTFunction) {
    singleParent(NTProgramPart);
    reduced = 1;
  } else if(curNode->type == NTDeclaration) {
    if(!prevNode || prevNode->type == NTProgramPart) {
      // independent declaration
      singleParent(NTProgramPart);
      reduced = 1;
    } else if(prevNode->type == NTStatement || (prevNode->type == NTTerminal
       && prevNode->token->type == TTLBrace)) {
      // independent declaration in block
      singleParent(NTStatement);
      reduced = 1;
    } else { // declaration part of FOR statement -- do nothing
    }
  } else if(isSubStatement(curNode->type)) {
    // Substatements: NTIfSt, NTNoop, NTNextSt, NTBreakSt, NTWhileSt,
    // NTMatchSt, NTLoopSt. 
    singleParent(NTStatement);
    reduced = 1;
  } else if(curNode->type == NTExpression) {
    reduced = reduceExpression();
  } else if(curNode->type == NTCallExpr) {
    if(!prevNode || prevNode->type == NTStatement
       || prevNode->type == NTProgramPart) { // TODO call statement (WRONG!)
      singleParent(NTStatement);
      reduced = 1;
    } else { // call expression 
      singleParent(NTExpression);
      reduced = 1;
    }
  } else if(curNode->type == NTTerm) {
    singleParent(NTExpression);
    reduced = 1;
  } else if(curNode->type == NTLiteral) {
    singleParent(NTExpression);
    reduced = 1;
  } else if(curNode->type == NTBinaryOp) {
  } else if(curNode->type == NTParam) {
    reduced = reduceParam();
  } else if(curNode->type == NTIdentifier) {
    reduced = reduceIdentifier();
  } else if(curNode->type == NTTerminal) {
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
    } else if(ttype == TTRBrace) {
      reduced = reduceRBrace();
    } else if((ttype == TTIncr || ttype == TTDecr) && laType == TTColon) {
      // ID++:   or   ID--:   in FOR statement 
      if(!prevNode) {
        parsErrorHelper("Beginning program with %s.",
          curNode, astFirstLeaf(curNode));
      }
      if(prevNode->type != NTIdentifier) {
        parsErrorHelper("Expected identifier before operator, found %s.",
          prevNode, astFirstLeaf(prevNode));
      }

      stackPop(2);
      Node* nodePtr = createAndPush(NTAssignment, 2, prevNode, curNode);
      reduced = 1;
    }
  }

  return reduced;
}

int reduceParam() {
  int reduced = 0;
  Node* prevNode = fromStackSafe(1);
  TokenType laType = lookAhead().type;

  if(!prevNode) {
    parsError(
      "Declaration of parameters at the beginning of the program.", 1, 1); 
  }

  if(laType == TTArrow) { // end of function parameters declaration
    Node* idNode = NULL;
    int idIndex = 1;
    int nParams = 1;

    while(1) {
      idNode = fromStackSafe(idIndex);
      if(!idNode) {
        Node* problematic = astFirstLeaf(prevNode);
        parsError("Bad declaration of parameters.", problematic->token->lnum, 
          problematic->token->chnum);
      }

      if(idNode->type == NTParam) nParams++;
      else if(idNode->type == NTIdentifier) {
        break;
      }
      idIndex++;
    }

    Node* nodePtr = newNode(NTParams);
    allocChildren(nodePtr, nParams);

    for(int i = 0; i < nParams; i++)
      nodePtr->children[i] = fromStackSafe(idIndex - (i * 2 + 1));

    stackPop(1 + (nParams - 1) * 2); 
    stackPush(nodePtr);
    reduced = 1;
  }
  return reduced;
}

int reduceRBrace() {
  int reduced = 0;
  Node* prevNode = NULL;

  int isBlock = 1;
  int lbraceIdx = 1;
  int nStatements = 0;

  while(1) {
    prevNode = fromStackSafe(lbraceIdx);
    if(!prevNode) { // error
      Node* problematic = astFirstLeaf(prevNode);
      parsError("Malformed block of statements.", problematic->token->lnum, 
        problematic->token->chnum);
    }

    if(prevNode->type == NTStatement) nStatements++;

    if(prevNode->type != NTStatement) {
      if(prevNode->type != NTTerminal || prevNode->token->type != TTLBrace) {
        isBlock = 0;
      }
      break;
    }
    lbraceIdx++;
  }

  if(isBlock) { // block of statements
    Node* nodePtr = newNode(NTStatement);
    allocChildren(nodePtr, nStatements);

    for(int i = 0; i < nStatements; i++)
      nodePtr->children[i] = fromStackSafe(lbraceIdx - (i + 1));

    stackPop(nStatements + 2); 
    stackPush(nodePtr);
    reduced = 1;
  } else { // match statement
  }
  
  return reduced;
}

int reduceStatement() {
  int reduced = 0;
  Node* curNode = fromStackSafe(0);
  Node* prevNode = fromStackSafe(1);
  TokenType laType = lookAhead().type;

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
    Node* iwmfNode = NULL;

    iwmfNode = fromStackSafe(3);
    if(!iwmfNode) {
      Node* problematic = astFirstLeaf(curNode);
      parsError("Before ':' and a statement, an 'if', 'while', "
        "'for' or 'match' construct is expected.", 
        problematic->token->lnum, problematic->token->chnum);
    }

    if(iwmfNode->type == NTTerminal && iwmfNode->token->type == TTIf) {
      Node* condNode = fromStackSafe(2);

      if(condNode->type != NTExpression) { // error
        parsErrorHelper("Unexpected %s as condition for 'if' statement.",
          condNode, astFirstLeaf(condNode));
      }

      // if statement
      if(laType == TTElse) { 
        // with else clause -- do nothing now
      } else { // without else
        stackPop(4);
        Node* nodePtr = createAndPush(NTIfSt, 2, condNode, curNode);
        reduced = 1;
      }
    } else if(iwmfNode->type == NTTerminal 
              && iwmfNode->token->type == TTWhile) {
      Node* whileNode = iwmfNode;
      Node* condNode = fromStackSafe(2);
      Node* colonNode = fromStackSafe(1);

      assertEqual(condNode, NTExpression, "");
      assertTokenEqual(colonNode, TTColon, "");

      stackPop(4);
      Node* nodePtr = createAndPush(NTWhileSt, 2, condNode, curNode);
      reduced = 1;
    } else if(iwmfNode->type == NTMatchClause
              || (iwmfNode->type == NTTerminal
              && iwmfNode->token->type == TTLBrace)) {
      // TODO match
    } else { // it has to be a FOR statement
      if(pStack.pointer < 7) {
        Node* problematic = astFirstLeaf(curNode);
        parsError("Before ':' and a statement, an 'if', 'while', "
          "'for' or 'match' construct is expected.", 
          problematic->token->lnum, problematic->token->chnum);
      }

      Node* forNode = fromStackSafe(7);
      Node* declNode = fromStackSafe(6);
      Node* comma1 = fromStackSafe(5);
      Node* exprNode = fromStackSafe(4);
      Node* comma2 = fromStackSafe(3);
      Node* assignNode = fromStackSafe(2);

      assertTokenEqual(forNode, TTFor, "Bad 'for' statement.");
      assertEqual(declNode, NTDeclaration, "Bad 'for' statement.");
      assertTokenEqual(comma1, TTComma, "Bad 'for' statement.");
      assertEqual(exprNode, NTExpression, "Bad 'for' statement.");
      assertTokenEqual(comma2, TTComma, "Bad 'for' statement.");
      assertEqual(assignNode, NTStatement, "Bad 'for' statement.");

      stackPop(8);
      Node* nodePtr = createAndPush(NTForSt, 4, 
        declNode, exprNode, assignNode, curNode);
      reduced = 1;
    }
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTElse) {
    if(pStack.pointer < 5) { // error: incomplete if statement
      Node* problematic = astFirstLeaf(prevNode);
      parsError("Malformed 'if' statement.", problematic->token->lnum, 
        problematic->token->chnum);
    }

    Node* thenNode = fromStackSafe(2);
    Node* condNode = fromStackSafe(4);
    Node* ifNode = fromStackSafe(5);

    assertEqual(thenNode, NTStatement, "Bad 'if' statement");
    assertEqual(condNode, NTExpression, "Bad 'if' statement");
    assertTokenEqual(ifNode, TTIf, "Bad 'if' statement");

    stackPop(6);
    Node* nodePtr = createAndPush(NTIfSt, 3, condNode, thenNode, curNode);
    reduced = 1;
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTLoop) {
    stackPop(2);
    Node* nodePtr = createAndPush(NTLoopSt, 1, curNode);
    reduced = 1;
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTArrow) {
    // TODO function declaration
    Node* prev3 = fromStackSafe(3);
    if(!prev3) {
      Node* problematic = astFirstLeaf(curNode);
      parsError("Bad function declaration.", 
        problematic->token->lnum, problematic->token->chnum);
    }

    if(prev3->type == NTTerminal && prev3->token->type == TTFunc) {
      // function declaration without parameters
      Node* idNode = fromStackSafe(2);
      assertEqual(idNode, NTIdentifier, "");

      // create an empty params node
      Node* paramsNode = newNode(NTParams);
      paramsNode->nChildren = 0;

      stackPop(4);
      Node* nodePtr = createAndPush(NTFunction, 3, idNode, paramsNode, curNode);
      reduced = 1;
    } else { // function declaration with parameters
      Node* prev4 = fromStackSafe(4);

      if(!prev4) {
        Node* problematic = astFirstLeaf(curNode);
        parsError("Bad function declaration.", 
          problematic->token->lnum, problematic->token->chnum);
      }

      Node* idNode = fromStackSafe(3);
      Node* paramsNode = fromStackSafe(2);
      assertTokenEqual(prev4, TTFunc, "Bad function declaration.");
      assertEqual(idNode, NTIdentifier, "Bad function declaration.");
      assertEqual(paramsNode, NTParams, "Bad function declaration.");

      stackPop(5);
      Node* nodePtr = createAndPush(NTFunction, 3, idNode, paramsNode, curNode);
      reduced = 1;
    }
  }

  return reduced;
}

int reduceRPar() {
  int reduced = 0;
  Node* curNode = fromStackSafe(0);
  Node* prevNode = fromStackSafe(1);
  Node* prevPrevNode = fromStackSafe(2);

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
      stackPush(prevNode);
      reduced = 1;
    }
  } else if(prevNode->type == NTCallParam ||
            (prevNode->type == NTTerminal && prevNode->token->type == TTLPar)) {
    reduced = reduceFunctionCallExpr();
  }

  return reduced;
}

int reduceFunctionCallSt() {
  int reduced = 0;
  Node* prevNode = NULL;
  prevNode = fromStackSafe(1);

  if(prevNode->type == NTCallParam) { // at least one param
    Node* idNode = prevNode;
    int nParams = 1;
    int idIndex = 1;

    while(idNode->type != NTIdentifier) { // find the function name
      idIndex++;
      idNode = fromStackSafe(idIndex);

      if(!idNode) { // this should never happen (as param checks for this)
        Node* problematic = astFirstLeaf(idNode);
        parsError("Malformed function call statement.", 
          problematic->token->lnum, problematic->token->chnum);
      }

      if(idNode->type == NTCallParam) {
        nParams++;
      } else if(idNode->type == NTIdentifier) {
        break;
      }
    }

    Node* nodePtr = newNode(NTCallSt);
    allocChildren(nodePtr, nParams + 1);
    nodePtr->children[0] = idNode;

    for(int i = 1; i <= nParams; i++)
      nodePtr->children[i] = fromStackSafe(idIndex - 1 - (i - 1) * 2);

    // we know there is at least one param
    // ID PARAM [, PARAM] ;
    stackPop(3 + (nParams - 1) * 2); 

    stackPush(nodePtr);
    reduced = 1;
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTLPar) {
    // ID();  -- function call statement without params
    Node* idNode = fromStackSafe(3);

    stackPop(4);
    Node* nodePtr = createAndPush(NTCallSt, 1, idNode);
    reduced = 1;
  }

  return reduced;
}

int reduceFunctionCallExpr() {
  int reduced = 0;
  Node* prevNode = NULL;
  prevNode = fromStackSafe(1);

  if(prevNode->type == NTCallParam) { // at least one param
    Node* idNode = prevNode;
    int nParams = 0;
    int idIndex = 0;

    while(idNode->type != NTIdentifier) { // find the function name
      nParams++;
      idIndex = (1 + nParams * 2);
      idNode = fromStackSafe(idIndex);

      if(!idNode) { // this should never happen (as param checks for this)
        Node* problematic = astFirstLeaf(prevNode);
        parsError("Malformed function call expression.", 
          problematic->token->lnum, problematic->token->chnum);
      }
    }

    Node* nodePtr = newNode(NTCallExpr);
    allocChildren(nodePtr, nParams + 1);
    nodePtr->children[0] = idNode;

    for(int i = 1; i <= nParams; i++)
      nodePtr->children[i] = fromStackSafe(idIndex - i * 2);

    stackPop(2 + nParams * 2);
    stackPush(nodePtr);
    reduced = 1;
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTLPar) {
    // ID()  -- function call without params
    Node* idNode = fromStackSafe(2);

    stackPop(3);
    Node* nodePtr = createAndPush(NTCallExpr, 1, idNode);
    reduced = 1;
  }

  return reduced;
}

int reduceIdentifier() {
  int reduced = 0;
  Node* curNode = fromStackSafe(0);
  Node* prevNode = fromStackSafe(1);
  TokenType laType = lookAhead().type;

  if(laType == TTId || laType == TTLPar || laType == TTNot 
     || isLiteral(laType)) 
  {
    // is function ID (will be reduced later)
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTFunc) {
    // is function ID (will be reduced later)
  }
  else { // is variable
    if(laType == TTIncr || laType == TTDecr || 
       laType == TTAdd || laType == TTSub || laType == TTAssign) {
      // assignment statements  -- do nothing
    }
    else if(prevNode && prevNode->type == NTType) // in declaration
    {
      Node* prevPrevNode = fromStackSafe(2);

      if(!prevPrevNode || prevPrevNode->type == NTProgramPart ||
         prevPrevNode->type == NTStatement || 
         (prevPrevNode->type == NTTerminal 
         && prevPrevNode->token->type == TTLBrace))
      {
        // variable declaration, will reduce later
      } else if(prevPrevNode->type == NTTerminal &&
                prevPrevNode->token->type == TTFor) { 
        // is part of for statement  -- do nothing
      } else { // is parameter
        stackPop(2);
        Node* nodePtr = createAndPush(NTParam, 2, prevNode, curNode);
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
  Node* curNode = fromStackSafe(0);
  Node* prevNode = fromStackSafe(1);
  Node* prevPrevNode = fromStackSafe(2);
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
      if(isExprTerminator(laType)) { // expression is finished
        singleParent(NTCallParam);
        reduced = 1;
      }
    } else if(prevPrevNode->type == NTDeclaration) { // for statement
      // do nothing
    } else { // unexpected node before: , EXPR
      parsErrorHelper("Expected parameter or declaration, found %s.",
        prevPrevNode, astFirstLeaf(prevPrevNode));
    }
  }
  else if(prevNode->type == NTIdentifier) {
    if(isExprTerminator(laType)) { // function call, parameter
      singleParent(NTCallParam);
      reduced = 1;
    }
  }
  else if(isExprTerminator(laType)) {
    if(prevNode->type == NTBinaryOp) {
      if(prevPrevNode && prevPrevNode->type != NTExpression) {
        if(prevNode->children[0]->token->type == TTMinus) { // - EXPR
          stackPop(2);
          Node* nodePtr = createAndPush(NTExpression, 2, prevNode, curNode);
          reduced = 1;
        } else {
          // If we see a OP EXPR sequence, there must be an EXPR before that 
          // (except if it is a minus)
          parsErrorHelper("Expected expression before operator, found %s.",
            prevPrevNode, astLastLeaf(prevPrevNode));
        }
      } else { // EXPR OP EXPR TERMINATOR sequence, EXPR OP EXPR => EXPR
        stackPop(3);
        Node* nodePtr = createAndPush(NTExpression, 3, 
          prevPrevNode, prevNode, curNode);
        reduced = 1;
      }
    } else if(prevNode->type == NTTerminal &&
              prevNode->token->type == TTNot) { // not EXPR
      stackPop(2);
      Node* nodePtr = createAndPush(NTExpression, 2, prevNode, curNode);
      reduced = 1;
    } else if(laType == TTComma) { // check FOR statement
      Node* forNode = fromStackSafe(4);

      if(forNode && forNode->type == NTTerminal 
         && forNode->token->type == TTFor) {
        Node* typeNode = fromStackSafe(3);

        assertEqual(typeNode, NTType, "");
        assertEqual(prevPrevNode, NTIdentifier, "");
        assertTokenEqual(prevNode, TTAssign, "");

/*
        if(typeNode->type != NTType) { // error
          parsErrorHelper("Expected type, found %s.",
            typeNode, astFirstLeaf(typeNode));
        }
        if(prevPrevNode->type != NTIdentifier) { // error
          parsErrorHelper("Expected identifier, found %s.",
            prevPrevNode, astFirstLeaf(prevPrevNode));
        }
        if(prevNode->type != NTTerminal || 
           prevNode->token->type != TTAssign) { // error
          parsErrorHelper("Expected symbol '=', found %s.",
            prevNode, astFirstLeaf(prevNode));
        }*/

        stackPop(4);
        Node* nodePtr = createAndPush(NTDeclaration, 3, 
          typeNode, prevPrevNode, curNode);
        reduced = 1;
      }
    } else if(laType == TTColon) { // EXPR in WHILE, IF, MATCH or FOR
      if(prevNode && prevNode->type == NTTerminal &&
         prevNode->token->type == TTWhile) { // while EXPR :
        // do nothing
      } else if(prevNode && prevNode->type == NTTerminal &&
                prevNode->token->type == TTIf) { // if EXPR :
      } else if(prevNode && (prevNode->type == NTMatchClause ||
                (prevNode->type == NTTerminal &&
                prevNode->token->type == TTLBrace))) {
      } else { // for DECL , EXPR , ... EXPR :
        Node* assignNode = fromStackSafe(1);
        Node* varNode = fromStackSafe(2);

        if(!assignNode) {
          Node* problematic = astFirstLeaf(curNode);
          parsError("Beginning program with expression.", 
            problematic->token->lnum, problematic->token->chnum);
        }

        if(!varNode) {
          Node* problematic = astFirstLeaf(curNode);
          parsError("Beginning program with assignment symbol.", 
            problematic->token->lnum, problematic->token->chnum);
        }

        if(assignNode->type != NTTerminal
           || isAssignmentOp(assignNode->token->type)) {
          parsErrorHelper(
            "Expected assignment operator before expression, found %s.",
            assignNode, astLastLeaf(assignNode));
        }

        if(varNode->type != NTIdentifier) {
          parsErrorHelper(
            "Expected identifier before assignment, found %s.",
            varNode, astFirstLeaf(varNode));
        }

        stackPop(3);
        Node* nodePtr = createAndPush(NTAssignment, 3, 
          varNode, assignNode, curNode);
        reduced = 1;
      }
    }
  } else if(isBinaryOp(laType)) {
    if(prevNode->type == NTBinaryOp) {
      if(prevPrevNode && prevPrevNode->type != NTExpression) {
        if(prevNode->children[0]->token->type == TTMinus) { // - EXPR
          stackPop(2);
          Node* nodePtr = createAndPush(NTExpression, 2, prevNode, curNode);
          reduced = 1;
        } else {
          // If we see a OP EXPR sequence, there must be an EXPR before that 
          // (except if it is a minus)
          parsErrorHelper("Expected expression before operator, found %s.",
            prevPrevNode, astLastLeaf(prevPrevNode));
        }
      } else if(precedence(laType) >= 
                precedence(prevNode->children[0]->token->type)){ 
        // EXPR OP EXPR OP sequence, reduce if the previous has precedence
        // (<= value for precedence()), otherwise do nothing
        stackPop(3);
        Node* nodePtr = createAndPush(NTExpression, 3,
          prevPrevNode, prevNode, curNode);
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
  Node* curNode = fromStackSafe(0);
  Node* prevNode = fromStackSafe(1);
  Node* prevPrevNode = fromStackSafe(2);

  if(!prevNode || prevNode->type == NTProgramPart) {
    singleParent(NTNoop);
    reduced = 1;
  } 
  else if(prevNode->type == NTTerminal) {
    TokenType ttype = prevNode->token->type;

    if(ttype == TTBreak || ttype == TTNext) {
      NodeType nType = NTBreakSt;
      if(ttype == TTNext) nType = NTNextSt;

      stackPop(2);
      Node* nodePtr = createAndPush(nType, 2, prevNode, curNode);
      reduced = 1;
    }
    else if(ttype == TTIncr || ttype == TTDecr) {
      assertEqual(prevPrevNode, NTIdentifier, "");  // ID++;   or   ID--;
      stackPop(3);
      Node* nodePtr = createAndPush(NTAssignment, 2, prevPrevNode, prevNode);
      reduced = 1;
    } else if(ttype == TTReturn) { // return ;
      stackPop(2);
      Node* nodePtr = createAndPush(NTReturnSt, 0);
      reduced = 1;
    }
  } 
  else if(prevNode->type == NTExpression) {
    if(prevPrevNode && prevPrevNode->type == NTTerminal) {
      if(prevPrevNode->token->type == TTAssign ||
         prevPrevNode->token->type == TTAdd ||
         prevPrevNode->token->type == TTSub) {
        // assignment or declaration with assignment
        Node* prev3 = fromStackSafe(3);
        Node* prev4 = fromStackSafe(4);

        if(prev3 && prev3->type == NTIdentifier) {
          if(!prev4) { // error
            char* format = "Assignment to undeclared variable '%s'.";
            int len = strlen(format) + prev3->children[0]->token->nameSize;
            char str[len];
            sprintf(str, format, prev3->children[0]->token->name);

            Node* problematic = astFirstLeaf(prev3);
            parsError(str, problematic->token->lnum, problematic->token->chnum);
          } 
          else if(prev4->type == NTProgramPart || prev4->type == NTStatement
            || (prev4->type == NTTerminal && (prev4->token->type == TTColon ||
            prev4->token->type == TTElse || prev4->token->type == TTLBrace))) 
          {
            // ID ASSIGN_OP EXPR ;  -- assignment
            stackPop(4);
            Node* nodePtr = createAndPush(NTAssignment, 3,
              prev3, prevPrevNode, prevNode);
            reduced = 1;
          }
          else if(prev4->type == NTType) { // declaration w/ assignment
            assertTokenEqual(prevPrevNode, TTAssign, "");
            // TYPE ID = EXPR ;  -- declaration with assignment
            stackPop(5);
            Node* nodePtr = createAndPush(NTDeclaration, 3,
              prev4, prev3, prevNode);
            reduced = 1;
          }
        } else { // error
          parsErrorHelper("Assignment to %s.", prev3, astFirstLeaf(prev3));
        }
      } else if(prevPrevNode->token->type == TTReturn) { // return statement
        stackPop(3);
        Node* nodePtr = createAndPush(NTReturnSt, 1, prevNode);
        reduced = 1;
      }
    }
  }
  else if(prevNode->type == NTCallParam) { // function call statement
    reduced = reduceFunctionCallSt();
  }
  else if(prevNode->type == NTIdentifier) { // uninitialized declaration
    assertEqual(prevPrevNode, NTType, "");
    stackPop(3);
    Node* nodePtr = createAndPush(NTDeclaration, 2, prevPrevNode, prevNode);
    reduced = 1;
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

  Node* nodePtr = newNode(NTProgram);
  allocChildren(nodePtr, pStack.pointer + 1);

  for(int i = 0; i <= pStack.pointer; i++) {
    nodePtr->children[i] = pStack.nodes[i];
  }

  stackPop(pStack.pointer + 1);
  stackPush(nodePtr);
}

void singleParent(NodeType type) {
  Node* curNode = fromStackSafe(0);
  stackPop(1);
  Node* nodePtr = createAndPush(type, 1, curNode);
}

int canPrecedeStatement(Node* node) {
  NodeType type = node->type;

  if(type == NTProgramPart || type == NTStatement) return 1;
  if(type == NTTerminal) {
    TokenType ttype = node->token->type;
    if(ttype == TTColon || ttype == TTComma || ttype == TTElse ||
       ttype == TTArrow || ttype == TTLBrace || ttype == TTLoop)
      return 1;
  }
  return 0;
}

void checkTree(Node* node) {
  int id = node->id;
  int type = node->type;
  int nChildren = node->nChildren;

  if(id < 0 || id > parserState.nodeCount)
    genericError("Internal memory error.");

  //printf("ID: %d, type: %d, nch: %d\n", node->id, 
  //  node->type, node->nChildren);

  for(int i = 0; i < node->nChildren; i++) {
    Node* chnode = node->children[i];
    //printf("ch[%d]: ID: %d, type: %d, nch: %d\n", i, chnode->id, 
    //  chnode->type, chnode->nChildren);
  }
  for(int i = 0; i < node->nChildren; i++) checkTree(node->children[i]);
}

