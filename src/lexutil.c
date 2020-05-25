/*
 *
 * Helper file for the lexer. Mostly printing/messaging/error functions and
 * character/token type functions.
 *
 */

#include "lexer.h"


int isWhitespace(char character) {
  if(character == ' ' || character == '\n' || character == '\t') return 1;
  return 0;
}

int isAlpha(char character) {
  if((character >= 'a' && character <= 'z') ||
    (character >= 'A' && character <= 'Z')) {
    return 1;
  }
  return 0;
}

int isNum(char character) {
  if(character >= '0' && character <= '9') return 1;
  return 0;
}

int startsDoubleOp(char character) {
  if(character == '=' || character == '+' || character == '-' ||
     character == '>' || character == '<') return 1;
  return 0;
}

int isSingleCharOp(char ch) {
  if(ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
     ch == ';' || ch == '*' || ch == '%' || ch == ':')
    return 1;
  return 0;
}

int isLiteral(TokenType type) {
  if(type == TTLitInt || type == TTLitFloat || type == TTLitString ||
     type == TTLitBool || type == TTLitArray)
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
     type == TTAnd || type == TTOr)
    return 1;
  return 0;
}

int precedence(TokenType type) {
  if(type == TTMult || type == TTDiv) return 0;
  return -1;
}

void printCharInFile(FILE* file, int lnum, int chnum) {
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

  printf("%s:%d:%d:\n\n", lexerState.filename, lnum, chnum);
  printf("%s", buff);
  printf("%s\n", buff_mark);
}

void printTokenInFile(FILE* file, Token token) {
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
  printf("%s:%d:%d:\n\n", lexerState.filename, token.lnum, token.chnum);
  printf("%s", buff);
  printf("%s\n", buff_mark);
}

void printFile(FILE* file) {
  rewind(file);
  char ch = fgetc(file);

  while(!feof(file)) {
    printf("%c", ch);
    ch = fgetc(file);
  }
  printf("\n");
  rewind(file);
}

void lexError(char* msg) {
  printf("ERROR: %s\n%s: line: %d, column: %d.\n", msg, lexerState.filename, 
    lexerState.lnum, lexerState.chnum);
  printCharInFile(lexerState.file, lexerState.lnum, lexerState.chnum);
  exit(1);
}
