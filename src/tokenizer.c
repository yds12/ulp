#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

#define BUFFER_SIZE 100000
#define TOKENS_SIZE 1000

/*
* Function prototypes
*/

char tokenizerGetChar(FILE* file);
void addToken(char* buffer, int size, TokenType type);
void eatIDKW(FILE* sourcefile, char* buffer);
void eatNumber(FILE* sourcefile, char* buffer);
void eatSlash(FILE* sourcefile, char* buffer); 
void eatDQuote(FILE* sourcefile, char* buffer);
void eatSingleSymb(FILE* sourcefile, char* buffer);
void eatDoubleSymb(FILE* sourcefile, char* buffer);

/*
* Functions
*/

void tokenizerStart(FILE* sourcefile, char* sourcefilename) {
  printFile(sourcefile);
  filename = sourcefilename;

  // The list of tokens in the source file
  tokens = (Token*) malloc(TOKENS_SIZE * sizeof(Token));

  tokenizerState = (TokenizerState) {
    .file = sourcefile,
    .lastChar = '\0',
    .lnum = 1,
    .chnum = 1
  };

  // Buffer to read the chars, is reset at the end of each token
  char charBuffer[BUFFER_SIZE];
  n_tokens = 0;

  // start with the first character
  if(!feof(sourcefile)) {
    charBuffer[0] = tokenizerGetChar(sourcefile);
  }

  while(!feof(sourcefile)) {
    char ch = charBuffer[0];

    // depending on the character read, call a method to process next token(s)
    // All functions should finish with the next character in buffer[0]
    if(ch == '/') eatSlash(sourcefile, charBuffer);
    else if(ch == '"') eatDQuote(sourcefile, charBuffer);
    else if(isSingleCharOp(ch)) eatSingleSymb(sourcefile, charBuffer);
    else if(belongsToDoubleOp(ch)) eatDoubleSymb(sourcefile, charBuffer);
    else if(isAlpha(ch) || ch == '_') eatIDKW(sourcefile, charBuffer);
    else if(isNum(ch)) eatNumber(sourcefile, charBuffer);
    else if(isWhitespace(ch)) {
      charBuffer[0] = tokenizerGetChar(sourcefile);
    }
  }

  // Debug info ---------
  for(int i = 0; i < n_tokens; i++) {
    //printf("\n\n");
    printf("tt_%d @%d,%d, len: %d, ||%s||\n", tokens[i].type, 
      tokens[i].lnum, tokens[i].chnum, tokens[i].nameSize, tokens[i].name);

    //printTokenInFile(sourcefile, tokens[i]);
  }
  printf("Total tokens: %d\n", n_tokens);
  // --------------------

  fclose(sourcefile);
}

/*
 * Process identifiers and keywords 
 */
void eatIDKW(FILE* sourcefile, char* buffer)
{
  int bufpos = 1;
  char ch = tokenizerGetChar(sourcefile);
  TokenType type = TTId;

  while(isNum(ch) || isAlpha(ch) || ch == '_') { // Still ID or keyword
    buffer[bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    bufpos++;
  }

  switch(bufpos) {
    case 2:
      if(strncmp("if", buffer, bufpos) == 0) type = TTIf;
      else if(strncmp("or", buffer, bufpos) == 0) type = TTOr;
      else if(strncmp("fn", buffer, bufpos) == 0) type = TTFunc;
      break;
    case 3:
      if(strncmp("and", buffer, bufpos) == 0) type = TTAnd;
      else if(strncmp("for", buffer, bufpos) == 0) type = TTFor;
      else if(strncmp("int", buffer, bufpos) == 0) type = TTInt;
      else if(strncmp("not", buffer, bufpos) == 0) type = TTNot;
      break;
    case 4:
      if(strncmp("bool", buffer, bufpos) == 0) type = TTBool;
      else if(strncmp("else", buffer, bufpos) == 0) type = TTElse;
      else if(strncmp("next", buffer, bufpos) == 0) type = TTNext;
      break;
    case 5:
      if(strncmp("break", buffer, bufpos) == 0) type = TTBreak;
      else if(strncmp("float", buffer, bufpos) == 0) type = TTFloat;
      else if(strncmp("while", buffer, bufpos) == 0) type = TTWhile;
      break;
    case 6:
      if(strncmp("string", buffer, bufpos) == 0) type = TTString;
      break;
  }

  // Add the ID/keyword token
  addToken(buffer, bufpos, type);
  buffer[0] = tokenizerState.lastChar;
  return;
}

void eatNumber(FILE* sourcefile, char* buffer)
{
  int bufpos = 1;
  char ch = tokenizerGetChar(sourcefile);
  TokenType type = TTLitInt;

  while(isNum(ch)) {
    buffer[bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    bufpos++;
  }

  if(ch == '.') { // float, read the rest of the number
    buffer[bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    bufpos++;

    if(isNum(ch)) {
      type = TTLitFloat;

      while(isNum(ch)) {
        buffer[bufpos] = ch;
        ch = tokenizerGetChar(sourcefile);
        bufpos++;
      }

    } else {
      error("Invalid number.");
    }
  }

  addToken(buffer, bufpos, type);
  buffer[0] = tokenizerState.lastChar;
  return;
}

/*
 * Process tokens constituted of two repeated characters,
 * such as ==, ++, --.
 */
void eatDoubleSymb(FILE* sourcefile, char* buffer)
{
  int bufpos = 1;
  char ch = tokenizerGetChar(sourcefile);
  TokenType type = TTUnknown;

  if(ch == buffer[0]) { // double symbol
    switch(buffer[0]) {
      case '=': type = TTEq;
        break;
      case '+': type = TTIncr;
        break;
      case '-': type = TTDecr;
        break;
    }

    buffer[bufpos] = ch;
    addToken(buffer, 2, type);
    buffer[0] = tokenizerGetChar(sourcefile);
  } else { // single symbol
    switch(buffer[0]) {
      case '=': type = TTAssign;
        break;
      case '+': type = TTPlus;
        break;
      case '-': type = TTMinus;
        break;
    }

    addToken(buffer, 1, type);
    buffer[0] = ch;
  }
  return;
}

/*
 * Process tokens constituted of a single character, like (, ), {, }, etc.
 */
void eatSingleSymb(FILE* sourcefile, char* buffer)
{
  int type = TTUnknown;

  switch(buffer[0]) {
    case '(': type = TTLPar;
      break;
    case ')': type = TTRPar;
      break;
    case '{': type = TTLBrace;
      break;
    case '}': type = TTRBrace;
      break;
    case ';': type = TTSemi;
      break;
    case '*': type = TTMult;
      break;
    case '%': type = TTMod;
      break;
    case ':': type = TTColon;
      break;
  }

  addToken(buffer, 1, type);
  buffer[0] = tokenizerGetChar(sourcefile);
  return;
}

// Need to improve string eating. Only valid ASCII or UTF-8 strings should
// be allowed
void eatDQuote(FILE* sourcefile, char* buffer)
{
  int bufpos = 1;
  char ch = tokenizerGetChar(sourcefile);

  while(ch != '\n' && ch != '"') { // read string till end
    buffer[bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    bufpos++;
  }

  if(ch == '\n') error("Line break in the middle of string.");

  // Here ch == '"'
  buffer[bufpos] = ch;
  addToken(buffer, bufpos + 1, TTLitString);
  buffer[0] = tokenizerGetChar(sourcefile);
  return;
}

void eatSlash(FILE* sourcefile, char* buffer)
{
  char ch = tokenizerGetChar(sourcefile);

  if(ch == '/') { // it was a comment
    tokenizerGetChar(sourcefile);

    // discard until newline
    while(tokenizerState.lastChar != '\n') tokenizerGetChar(sourcefile);
    buffer[0] = tokenizerGetChar(sourcefile);
  } else { // not a comment, thus division
    addToken(buffer, 1, TTDiv); // adds division op.
    buffer[0] = ch;
  }
  return;
}

void addToken(char * buffer, int size, TokenType type) {
  if(size == 0) return;
  if(size == 1 && isWhitespace(buffer[0])) return;

  int lnum, chnum;

  if(tokenizerState.lastChar == '\n') {
    lnum = tokenizerState.prevLnum;
    chnum = tokenizerState.prevChnum - size - 1;
  } else {
    lnum = tokenizerState.lnum;
    chnum = tokenizerState.chnum - size - 1;
  }

  char* tokenName = (char*) malloc((size + 1) * sizeof(char));
  Token token = { tokenName, size, type, lnum, chnum };
  strncpy(tokenName, buffer, size);
  token.name[size] = '\0';

  if(n_tokens >= TOKENS_SIZE) {
    error("Too many tokens.");
  }
  tokens[n_tokens] = token;
  n_tokens++;
}

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

int belongsToDoubleOp(char character) {
  if(character == '=' || character == '+' || character == '-') return 1;
  return 0;
}

int isSingleCharOp(char ch) {
  if(ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
     ch == ';' || ch == '*' || ch == '%' || ch == ':')
    return 1;
  return 0;
}

char tokenizerGetChar(FILE* file) {
  if(!feof(file)) {
    char ch = fgetc(file);
    tokenizerState.lastChar = ch;
    tokenizerState.prevLnum = tokenizerState.lnum;
    tokenizerState.prevChnum = tokenizerState.chnum;

    if(ch == '\n') {
      tokenizerState.lnum++;
      tokenizerState.chnum = 1;
    } else {
      tokenizerState.chnum++;
    }
    return ch;
  }
  else return EOF;
}

void printTokenInFile(FILE* file, Token token) {
  rewind(file);

  int BUFF_SIZE = 80;
  char buff[BUFF_SIZE + 1];
  char buff_mark[BUFF_SIZE + 1];

  for(int i = 0; i < BUFF_SIZE; i++) {
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

void error(char* msg) {
  printf("ERROR: %s\n%s: line: %d, column: %d.\n", msg, filename, 
    tokenizerState.lnum, tokenizerState.chnum);
  exit(1);
}
