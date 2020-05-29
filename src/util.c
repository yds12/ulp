
#include "util.h"

int isLiteral(TokenType type) {
  if(type == TTLitInt || type == TTLitFloat || type == TTLitString ||
     type == TTLitBool || type == TTLitArray || type == TTTrue ||
     type == TTFalse)
    return 1;
  return 0;
}

int isType(TokenType type) {
  if(type == TTInt || type == TTBool || type == TTString ||
     type == TTFloat)
    return 1;
  return 0;
}

int isBinaryOp(TokenType type) {
  if(type == TTEq || type == TTPlus || type == TTMinus ||
     type == TTMult || type == TTDiv || type == TTGreater ||
     type == TTLess || type == TTGEq || type == TTLEq ||
     type == TTAnd || type == TTOr || type == TTMod)
    return 1;
  return 0;
}

int precedence(TokenType type) {
  if(type == TTMult || type == TTDiv) return 0;
  if(type == TTPlus || type == TTMinus) return 1;
  if(type == TTMod) return 1;
  if(type == TTNot) return 3;
  if(type == TTAnd || type == TTOr) return 4;
  if(type == TTGreater || type == TTGEq || type == TTEq || 
     type == TTLess || type == TTLEq) return 5;
  return -1;
}

int isExprTerminator(TokenType type) {
  if(type == TTColon || type == TTComma 
     || type == TTSemi || type == TTRPar) return 1;
  return 0; 
}

void printTokenInFile(FILE* file, char* filename, Token token) {
  rewind(file);

  int BUFF_SIZE = 80;
  char buff[BUFF_SIZE + 1];
  char buff_mark[BUFF_SIZE + 1];

  for(int i = 0; i <= BUFF_SIZE; i++) {
    buff[i] = '\0';
    buff_mark[i] = '\0';
  }

  int lnum = 1;
  int chnum = 1;
  while(!feof(file)) {
    char ch = fgetc(file);

    if(lnum == token.lnum) {
      if(chnum < BUFF_SIZE) buff[chnum - 1] = ch;
    } else if(lnum > token.lnum) break;

    if(ch == '\n') {
      lnum++;
      chnum = 1;
    } else chnum++;
  }

  for(int i = 0; i < BUFF_SIZE; i++) {
    if(i + 1 < token.chnum || i + 1 >= token.chnum + token.nameSize) {
      buff_mark[i] = ' '; 
    }
    else buff_mark[i] = '^';
  }

  printf("\nToken '%s':\n", token.name);
  printf("%s:%d:%d:\n\n", filename, token.lnum, token.chnum);
  printf("%s", buff);
  printf("%s\n", buff_mark);
}

void printCharInFile(FILE* file, char* filename, int lnum, int chnum) {
  rewind(file);

  int BUFF_SIZE = 80;
  char buff[BUFF_SIZE + 1];
  char buff_mark[BUFF_SIZE + 1];

  for(int i = 0; i <= BUFF_SIZE; i++) {
    buff[i] = '\0';
    buff_mark[i] = '\0';
  }

  int _lnum = 1;
  int _chnum = 1;
  while(!feof(file)) {
    char ch = fgetc(file);

    if(_lnum == lnum) {
      if(_chnum < BUFF_SIZE) buff[_chnum - 1] = ch;
    } else if(_lnum > lnum) break;

    if(ch == '\n') {
      _lnum++;
      _chnum = 1;
    } else _chnum++;
  }

  for(int i = 0; i < BUFF_SIZE; i++) {
    if(i + 1 < chnum || i + 1 >= chnum + 1) {
      buff_mark[i] = ' '; 
    }
    else buff_mark[i] = '^';
  }

  printf("%s: line %d, column %d:\n\n", filename, lnum, chnum);
  printf("%s", buff);
  printf("%s\n", buff_mark);
}

void strReplaceNodeAndTokenName(char* str, char* format, Node* node) {
  NodeType type = node->type;

  if(type == NTTerminal) {
    char strToken[node->token->nameSize + 10];
    char* strTokenFormat = "token '%s'";
    sprintf(strToken, strTokenFormat, node->token->name);
    sprintf(str, format, strToken);
  } else strReplaceNodeName(str, format, type); 
}

void strReplaceTokenName(char* str, char* format, TokenType ttype) {
  switch(ttype) {
    case TTId: sprintf(str, format, "identifier"); break;

    // literals
    case TTLitInt: sprintf(str, format, "integer literal"); break;
    case TTLitFloat: sprintf(str, format, "float literal"); break;
    case TTLitString: sprintf(str, format, "string literal"); break;
    case TTLitBool: sprintf(str, format, "boolean literal"); break;
    case TTLitArray: sprintf(str, format, "array literal"); break;

    // structural
    case TTLPar: sprintf(str, format, "token '('"); break;
    case TTRPar: sprintf(str, format, "token ')'"); break;
    case TTLBrace: sprintf(str, format, "token '{'"); break;
    case TTRBrace: sprintf(str, format, "token '}'"); break;
    case TTSemi: sprintf(str, format, "token ';'"); break;
    case TTColon: sprintf(str, format, "token ':'"); break;
    case TTComma: sprintf(str, format, "token ','"); break;
    case TTArrow: sprintf(str, format, "token '=>'"); break;

    // operators
    case TTDiv: sprintf(str, format, "operator '/'"); break;
    case TTPlus: sprintf(str, format, "operator '+'"); break;
    case TTMinus: sprintf(str, format, "operator '-'"); break;
    case TTMod: sprintf(str, format, "operator '%'"); break;
    case TTMult: sprintf(str, format, "operator '*'"); break;
    case TTGreater: sprintf(str, format, "operator '>'"); break;
    case TTGEq: sprintf(str, format, "operator '>='"); break;
    case TTLess: sprintf(str, format, "operator '<'"); break;
    case TTLEq: sprintf(str, format, "operator '<='"); break;
    case TTEq: sprintf(str, format, "operator '=='"); break;
    case TTAssign: sprintf(str, format, "operator '='"); break;
    case TTIncr: sprintf(str, format, "operator '++'"); break;
    case TTDecr: sprintf(str, format, "operator '--'"); break;
    case TTAdd: sprintf(str, format, "operator '+='"); break;
    case TTSub: sprintf(str, format, "operator '-='"); break;

    // keywords
    case TTIf: sprintf(str, format, "keyword 'if'"); break;
    case TTElse: sprintf(str, format, "keyword 'else'"); break;
    case TTFor: sprintf(str, format, "keyword 'for'"); break;
    case TTFunc: sprintf(str, format, "keyword 'fn'"); break;
    case TTWhile: sprintf(str, format, "keyword 'while'"); break;
    case TTNot: sprintf(str, format, "keyword 'not'"); break;
    case TTNext: sprintf(str, format, "keyword 'next'"); break;
    case TTBreak: sprintf(str, format, "keyword 'break'"); break;
    case TTInt: sprintf(str, format, "keyword 'int'"); break;
    case TTString: sprintf(str, format, "keyword 'string'"); break;
    case TTBool: sprintf(str, format, "keyword 'bool'"); break;
    case TTFloat: sprintf(str, format, "keyword 'float'"); break;
    case TTAnd: sprintf(str, format, "keyword 'and'"); break;
    case TTOr: sprintf(str, format, "keyword 'or'"); break;
    case TTReturn: sprintf(str, format, "keyword 'return'"); break;;
    case TTLoop: sprintf(str, format, "keyword 'loop'"); break;
    case TTMatch: sprintf(str, format, "keyword 'match'"); break;
    default: sprintf(str, format, "other token");
  }
}

void strReplaceNodeName(char* str, char* format, NodeType type) {
  switch(type) {
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
    case NTCallExpr: sprintf(str, format, "function call");
      break;
    case NTCallParam: sprintf(str, format, "parameter expression");
      break;
    case NTAssignment: sprintf(str, format, "assignment");
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
    case NTProgramPart: sprintf(str, format, "PP");
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
    case NTCallExpr: sprintf(str, format, "CALL");
      break;
    case NTCallParam: sprintf(str, format, "C PARAM");
      break;
    case NTAssignment: sprintf(str, format, "ASSIGN");
      break;
    default: sprintf(str, format, "NT");
      break;
  }
}

