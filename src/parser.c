#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

#define DEBUG

void shift();
int reduce();
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

#ifdef DEBUG
printStack();  
#endif

      success = reduce(); 
    } while(success);
  }

#ifdef DEBUG
  graphvizAst(pStack.nodes[pStack.pointer]);
#endif
}

void shift() {
  Token* token = &parserState.tokens[parserState.nextToken];
  parserState.nextToken++;
  Node* createdNode = createAndPush(NTTerminal, 0);
  createdNode->token = token;
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
    reduced = reduceStatement();
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
       || prevNode->type == NTProgramPart) { // call statement (WRONG!)
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
    } else if(ttype == TTRBrace) {
      reduced = reduceRBrace();
    } else if((ttype == TTIncr || ttype == TTDecr) 
              && lookAhead().type == TTColon) {
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
      Node* nodePtr = createAndPush(NTAssignment, 2);
      nodePtr->children[0] = prevNode;
      nodePtr->children[1] = curNode;
      reduced = 1;
    }
  }

  return reduced;
}

int reduceRBrace() {
  int reduced = 0;
  Node* prevNode = NULL;

  int isBlock = 1;
  int lbraceIdx = 1;

  while(1) {
    if(pStack.pointer >= lbraceIdx)
      prevNode = pStack.nodes[pStack.pointer - lbraceIdx];
    else { // error
      Node* problematic = astFirstLeaf(prevNode);
      parsError("Malformed block of statements.", problematic->token->lnum, 
        problematic->token->chnum);
    }

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
    allocChildren(nodePtr, lbraceIdx - 1);

    for(int i = 0; i < lbraceIdx - 1; i++)
      nodePtr->children[i] = pStack.nodes[lbraceIdx + 1 + i];

    stackPop(lbraceIdx + 1); 
    stackPush(nodePtr);
    reduced = 1;
  } else { // match statement
  }
  
  return reduced;
}

int reduceStatement() {
  int reduced = 0;
  Node* curNode = pStack.nodes[pStack.pointer];
  Node* prevNode = NULL;
  if(pStack.pointer >= 1) prevNode = pStack.nodes[pStack.pointer - 1];

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

    if(pStack.pointer >= 3) iwmfNode = pStack.nodes[pStack.pointer - 3];
    else {
      Node* problematic = astFirstLeaf(curNode);
      parsError("Before ':' and a statement, an 'if', 'while', "
        "'for' or 'match' construct is expected.", 
        problematic->token->lnum, problematic->token->chnum);
    }

    if(iwmfNode->type == NTTerminal && iwmfNode->token->type == TTIf) {
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
        Node* nodePtr = createAndPush(NTIfSt, 2);
        nodePtr->children[0] = condNode;
        nodePtr->children[1] = curNode;
        reduced = 1;
      }
    } else if(iwmfNode->type == NTTerminal 
              && iwmfNode->token->type == TTWhile) {
      // TODO while
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

      // TODO Create assert functions to simplify this
      Node* forNode = pStack.nodes[pStack.pointer - 7];
      Node* declNode = pStack.nodes[pStack.pointer - 6];
      Node* comma1 = pStack.nodes[pStack.pointer - 5];
      Node* exprNode = pStack.nodes[pStack.pointer - 4];
      Node* comma2 = pStack.nodes[pStack.pointer - 3];
      Node* assignNode = pStack.nodes[pStack.pointer - 2];

      if(forNode->type != NTTerminal || forNode->token->type != TTFor) {
        parsErrorHelper("Expected 'for' keyword, found %s.", 
          forNode, astFirstLeaf(forNode));
      }
      if(declNode->type != NTDeclaration) {
        parsErrorHelper("Declaration expected after 'for' keyword, found %s.", 
          declNode, astFirstLeaf(declNode));
      }
      if(comma1->type != NTTerminal || comma1->token->type != TTComma) {
        parsErrorHelper("Expected ',' after 'for' declaration, found %s.", 
          comma1, astFirstLeaf(comma1));
      }
      if(exprNode->type != NTExpression) {
        parsErrorHelper(
          "Conditional expression expected after 'for' declaration, found %s.", 
          exprNode, astFirstLeaf(exprNode));
      }
      if(comma2->type != NTTerminal || comma2->token->type != TTComma) {
        parsErrorHelper("Expected ',' after 'for' condition, found %s.", 
          comma2, astFirstLeaf(comma2));
      }
      if(assignNode->type != NTStatement) {
        parsErrorHelper(
          "Assignment statement expected after 'for' condition, found %s.", 
          assignNode, astFirstLeaf(assignNode));
      }

      stackPop(8);
      Node* nodePtr = createAndPush(NTForSt, 4);
      nodePtr->children[0] = declNode;
      nodePtr->children[1] = exprNode;
      nodePtr->children[2] = assignNode;
      nodePtr->children[3] = curNode;
      reduced = 1;
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
    Node* nodePtr = createAndPush(NTIfSt, 3);
    nodePtr->children[0] = condNode;
    nodePtr->children[1] = thenNode;
    nodePtr->children[2] = curNode;
    reduced = 1;
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
      Node* nodePtr = createAndPush(NTExpression, 1);
      nodePtr->children[0] = prevNode; // parentheses are ignored
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
  prevNode = pStack.nodes[pStack.pointer - 1];

  if(prevNode->type == NTCallParam) { // at least one param
    Node* idNode = prevNode;
    int nParams = 1;
    int idIndex = 1;

    while(idNode->type != NTIdentifier) { // find the function name
      idIndex++;

      if(pStack.pointer >= idIndex) {
        idNode = pStack.nodes[pStack.pointer - idIndex];
      } else { // this should never happen (as param checks for this)
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
      nodePtr->children[i] = pStack.nodes[idIndex + i * 2];

    // we know there is at least one param
    // ID PARAM [, PARAM] ;
    stackPop(3 + (nParams - 1) * 2); 

    stackPush(nodePtr);
    reduced = 1;
  } else if(prevNode->type == NTTerminal && prevNode->token->type == TTLPar) {
    // ID();  -- function call statement without params
    Node* idNode = pStack.nodes[pStack.pointer - 3];

    stackPop(4);
    Node* nodePtr = createAndPush(NTCallSt, 1);
    nodePtr->children[0] = idNode; // parentheses are ignored
    reduced = 1;
  }

  return reduced;
}

int reduceFunctionCallExpr() {
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

    Node* nodePtr = newNode(NTCallExpr);
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
    Node* nodePtr = createAndPush(NTCallExpr, 1);
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
      Node* prevPrevNode = NULL;
      if(pStack.pointer >= 2) prevPrevNode = pStack.nodes[pStack.pointer - 2];

      if(!prevPrevNode || prevPrevNode->type == NTProgramPart ||
         prevPrevNode->type == NTStatement)
      {
        // variable declaration, will reduce later
      } else if(prevPrevNode->type == NTTerminal &&
                prevPrevNode->token->type == TTFor) { 
        // is part of for statement  -- do nothing
      } else { // is parameter
        stackPop(2);
        Node* nodePtr = createAndPush(NTParam, 2);
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
  else if(prevNode->type == NTIdentifier) { // function call, parameter
    singleParent(NTCallParam);
    reduced = 1;
  }
  else if(isExprTerminator(laType)) {
    if(prevNode->type == NTBinaryOp) {
      if(prevPrevNode && prevPrevNode->type != NTExpression) {
        if(prevNode->children[0]->token->type == TTMinus) { // - EXPR
          stackPop(2);
          Node* nodePtr = createAndPush(NTExpression, 2);
          nodePtr->children[0] = prevNode;
          nodePtr->children[1] = curNode;
          reduced = 1;
        } else {
          // If we see a OP EXPR sequence, there must be an EXPR before that 
          // (except if it is a minus)
          parsErrorHelper("Expected expression before operator, found %s.",
            prevPrevNode, astLastLeaf(prevPrevNode));
        }
      } else { // EXPR OP EXPR TERMINATOR sequence, EXPR OP EXPR => EXPR
        stackPop(3);
        Node* nodePtr = createAndPush(NTExpression, 3);
        nodePtr->children[0] = prevPrevNode;
        nodePtr->children[1] = prevNode;
        nodePtr->children[2] = curNode;
        reduced = 1;
      }
    } else if(laType == TTComma) { // check FOR statement
      Node* forNode = NULL;
      if(pStack.pointer >= 4) forNode = pStack.nodes[pStack.pointer - 4];

      if(forNode && forNode->type == NTTerminal 
         && forNode->token->type == TTFor) {
        Node* typeNode = pStack.nodes[pStack.pointer - 3];
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
        }

        stackPop(4);
        Node* nodePtr = createAndPush(NTDeclaration, 3);
        nodePtr->children[0] = typeNode;
        nodePtr->children[1] = prevPrevNode;
        nodePtr->children[2] = curNode;
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
        Node* assignNode = NULL;
        Node* varNode = NULL;
        if(pStack.pointer >= 1) assignNode = pStack.nodes[pStack.pointer - 1];
        if(pStack.pointer >= 2) varNode = pStack.nodes[pStack.pointer - 2];

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
        Node* nodePtr = createAndPush(NTAssignment, 3);
        nodePtr->children[0] = varNode;
        nodePtr->children[1] = assignNode;
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
        Node* nodePtr = createAndPush(NTExpression, 3);
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
  } 
  else if(prevNode->type == NTTerminal) {
    TokenType ttype = prevNode->token->type;

    if(ttype == TTBreak || ttype == TTNext) {
      NodeType nType = NTBreakSt;
      if(ttype == TTNext) nType = NTNextSt;

      stackPop(2);
      Node* nodePtr = createAndPush(nType, 2);
      nodePtr->children[0] = prevNode;
      nodePtr->children[1] = curNode;
      reduced = 1;
    }
  } 
  else if(prevNode->type == NTExpression) {
    if(prevPrevNode && prevPrevNode->type == NTTerminal) {
      if(prevPrevNode->token->type == TTAssign) {
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
          } 
          else if(prev4->type == NTProgramPart || prev4->type == NTStatement) 
          {
            // ID = EXPR ;  -- assignment
            stackPop(4);
            Node* nodePtr = createAndPush(NTAssignment, 2);
            nodePtr->children[0] = prev3; // ID
            nodePtr->children[1] = prevNode; // EXPR
            reduced = 1;
          }
          else if(prev4->type == NTType) { // declaration w/ assignment
            // TYPE ID = EXPR ;  -- declaration with assignment
            stackPop(5);
            Node* nodePtr = createAndPush(NTDeclaration, 3);
            nodePtr->children[0] = prev4; // TYPE
            nodePtr->children[1] = prev3; // ID
            nodePtr->children[2] = prevNode; // EXPR
            reduced = 1;
          }
        } else { // error
          parsErrorHelper("Assignment to %s.",
            prev3, astFirstLeaf(prev3));
        }
      } else if(prevPrevNode->token->type == TTReturn) { // return statement
        stackPop(3);
        Node* nodePtr = createAndPush(NTReturnSt, 1);
        nodePtr->children[0] = prevNode;
        reduced = 1;
      }
    }
  }
  else if(prevNode->type == NTCallParam) { // function call statement
    reduced = reduceFunctionCallSt();
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
  Node* curNode = pStack.nodes[pStack.pointer];
  stackPop(1);
  Node* nodePtr = createAndPush(type, 1);
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

