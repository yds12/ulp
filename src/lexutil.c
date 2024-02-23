/*
 *
 * Helper file for the lexer. Mostly printing/messaging/error functions and
 * character/token type functions.
 *
 */

#include "cli.h"
#include "util.h"
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

int isSingleCharToken(char ch) {
  if(ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
     ch == ';' || ch == '*' || ch == '%' || ch == ':' ||
     ch == ',')
    return 1;
  return 0;
}

void printFile(FILE* file) {
  if(file == stdin) return;
  if(cli.outputType > OUT_DEBUG) return;

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
  if(cli.outputType > OUT_DEFAULT) exit(1);

  fprintf(stderr, "\nLexical " ERROR_COLOR_START "ERROR" COLOR_END
    ": %s\n%s: line: %d, column: %d.\n", msg,
    lexerState.filename, lexerState.lnum, lexerState.chnum);
  printCharInFile(lexerState.file, lexerState.filename,
    lexerState.lnum, lexerState.chnum);
  exit(1);
}

void printTokens() {
  if(cli.outputType != OUT_DEBUG) return;

  for(int i = 0; i < lexerState.nTokens; i++) {
    Token* t = lexerState.tokens[i];
    printf("\n\n||%s||, type:%d, pos:%d,%d\n",
      t->name, t->type, t->lnum, t->chnum);
    printTokenInFile(lexerState.file, lexerState.filename, t);
  }
}

