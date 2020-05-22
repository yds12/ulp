#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

#define BUFFER_SIZE 100000
#define TOKENS_SIZE 1000

/*
* Function prototypes
*/

char* filename;

char tokenizerGetChar(FILE* file);

void printFile(FILE* file);

void printTokenInFile(FILE* file, Token token);

void error(char* msg, int lnum, int chnum);

void addToken(char * buffer, int size, TokenType type);

int is_whitespace(char character);

int is_alpha(char character);

int is_num(char character);

void processIDKW(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum);

void processNumber(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum);

// for straightforward 1 character symbols: *, {, }, (, )
void processSingleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum);

// for symbols that are duplicated characters: ==, ++, --
void processDoubleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum);

void processSlash(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum);

void processDQuote(FILE* sourcefile, char* buffer, int* bufpos,
  int* lnum, int* chnum);


/*
* Functions
*/

void tokenizer_start(FILE* sourcefile, char* sourcefilename) {
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
  char char_buffer[BUFFER_SIZE];
  int i = 0;
  n_tokens = 0;
  char ch;

  while(!feof(sourcefile)) {
    ch = tokenizerGetChar(sourcefile);

    char_buffer[i] = ch;

    if(ch == '/') { // beginning of a comment or division
      processSlash(sourcefile, char_buffer, &i,
        &tokenizerState.lnum, &tokenizerState.chnum);
    } else if(ch == '"') { // beginning of a string
      processDQuote(sourcefile, char_buffer, &i,
        &tokenizerState.lnum, &tokenizerState.chnum);
    } else if(ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
      ch == ';' || ch == '*' || ch == '%' || ch == ':') 
    {
      processSingleSymb(sourcefile, char_buffer, &i,
        &tokenizerState.lnum, &tokenizerState.chnum);
    } else if(ch == '=' || ch == '+' || ch == '-') {
      processDoubleSymb(sourcefile, char_buffer, &i,
        &tokenizerState.lnum, &tokenizerState.chnum);
    } else if(is_alpha(ch) || ch == '_') { // ID or keyword
      processIDKW(sourcefile, char_buffer, &i,
        &tokenizerState.lnum, &tokenizerState.chnum);
    } else if(is_num(ch)) { // number
      processNumber(sourcefile, char_buffer, &i,
        &tokenizerState.lnum, &tokenizerState.chnum);
    } else if(is_whitespace(ch)) {
      if(i > 0) { // end of a token
        addToken(char_buffer, i, TTUnknown);
      }
      i = 0; // restart buffer
    } else i++;

    if(i == BUFFER_SIZE) {
      error("File too long. Buffer overflowed.", 
        tokenizerState.lnum, tokenizerState.chnum);
    }
  }

  // Debug info ---------
  for(int i = 0; i < n_tokens; i++) {
    printf("\n\ntt_%d @%d,%d, len: %d, ||%s||\n", tokens[i].type, 
      tokens[i].lnum, tokens[i].chnum, tokens[i].name_size, tokens[i].name);

    printTokenInFile(sourcefile, tokens[i]);
  }
  printf("Total tokens: %d\n", n_tokens);
  // --------------------

  fclose(sourcefile);
}

void processIDKW(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown);
    buffer[0] = buffer[*bufpos];
  }
  *bufpos = 1;

  char ch = tokenizerGetChar(sourcefile);

  TokenType type = TTId;

  while(is_num(ch) || is_alpha(ch) || ch == '_') { // Still ID or keyword
    buffer[*bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    (*bufpos)++;
  }

  switch(*bufpos) {
    case 2:
      if(strncmp("if", buffer, *bufpos) == 0) type = TTIf;
      else if(strncmp("or", buffer, *bufpos) == 0) type = TTOr;
      else if(strncmp("fn", buffer, *bufpos) == 0) type = TTFunc;
      break;
    case 3:
      if(strncmp("and", buffer, *bufpos) == 0) type = TTAnd;
      else if(strncmp("for", buffer, *bufpos) == 0) type = TTFor;
      else if(strncmp("int", buffer, *bufpos) == 0) type = TTInt;
      else if(strncmp("not", buffer, *bufpos) == 0) type = TTNot;
      break;
    case 4:
      if(strncmp("bool", buffer, *bufpos) == 0) type = TTBool;
      else if(strncmp("else", buffer, *bufpos) == 0) type = TTElse;
      else if(strncmp("next", buffer, *bufpos) == 0) type = TTNext;
      break;
    case 5:
      if(strncmp("break", buffer, *bufpos) == 0) type = TTBreak;
      else if(strncmp("float", buffer, *bufpos) == 0) type = TTFloat;
      else if(strncmp("while", buffer, *bufpos) == 0) type = TTWhile;
      break;
    case 6:
      if(strncmp("string", buffer, *bufpos) == 0) type = TTString;
      break;
  }

  // Add the ID/keyword token
  addToken(buffer, *bufpos, type);

  // we have read already the first character of the next token
  // during the while loop above
  if(is_whitespace(ch)) { // discard if whitespace
    *bufpos = 0;
  } else {
    buffer[0] = ch;
    *bufpos = 1;
  }
}

void processNumber(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown);
    buffer[0] = buffer[*bufpos];
  }
  *bufpos = 1;

  char ch = tokenizerGetChar(sourcefile);

  TokenType type = TTLitInt;

  while(is_num(ch)) {
    buffer[*bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    (*bufpos)++;
  }

  if(ch == '.') { // float, read the rest of the number
    buffer[*bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    (*bufpos)++;

    if(is_num(ch)) {
      type = TTLitFloat;

      while(is_num(ch)) {
        buffer[*bufpos] = ch;
        ch = tokenizerGetChar(sourcefile);
        (*bufpos)++;
      }

    } else {
      error("Invalid number.", *lnum, *chnum);
    }
  }

  addToken(buffer, *bufpos, type);

  // we have read already the first character of the next token
  // during the while loop above
  if(is_whitespace(ch)) { // discard if whitespace
    *bufpos = 0;
  } else {
    buffer[0] = ch;
    *bufpos = 1;
  }
}

void processDoubleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown);
    buffer[0] = buffer[*bufpos];
  }
  *bufpos = 1;

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

    buffer[*bufpos] = ch;
    addToken(buffer, 2, type);
    *bufpos = 0;
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
    *bufpos = 1;
  }
}

void processSingleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown);
    buffer[0] = buffer[*bufpos];
    *bufpos = 1;
  }
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

  addToken(buffer, *bufpos, type);
  *bufpos = 0;
}

// Need to improve string processing. Only valid ASCII or UTF-8 strings should
// be allowed
void processDQuote(FILE* sourcefile, char* buffer, int* bufpos,
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown);
    buffer[0] = buffer[*bufpos];
    *bufpos = 1;
  }

  char ch = tokenizerGetChar(sourcefile);
  while(ch != '\n' && ch != '"') { // read string till end
    buffer[*bufpos] = ch;
    ch = tokenizerGetChar(sourcefile);
    (*bufpos)++;
  }

  if(ch == '\n') { // error: broke line in the middle of string
    error("Line break in the middle of string.", *lnum, *chnum);
  } else if(ch == '"') {
    addToken(buffer, *bufpos, TTLitString);
    *bufpos = 0;
  }
}

void processSlash(FILE* sourcefile, char* buffer, int* bufpos,
  int* lnum, int* chnum)
{
  char ch = tokenizerGetChar(sourcefile);

  if(ch == '/') { // it was a comment
    ch = tokenizerGetChar(sourcefile);

    // discard until newline
    while(ch != '\n') ch = tokenizerGetChar(sourcefile);
  } else if(*bufpos > 0) { // not a comment, thus division
    // adds previous token
    addToken(buffer, *bufpos, TTUnknown);

    // adds division op.
    addToken(buffer + (*bufpos), 1, TTDiv);
  }
  *bufpos = 0; // resets buffer
  return;
}

void addToken(char * buffer, int size, TokenType type) {
  if(size == 0) return;

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

  tokens[n_tokens] = token;
  n_tokens++;
}

int is_whitespace(char character) {
  if(character == ' ' || character == '\n' || character == '\t') return 1;
  return 0;
}

int is_alpha(char character) {
  if((character >= 'a' && character <= 'z') ||
    (character >= 'A' && character <= 'Z')) {
    return 1;
  }
  return 0;
}

int is_num(char character) {
  if(character >= '0' && character <= '9') return 1;
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
    if(i + 1 < token.chnum || i + 1 >= token.chnum + token.name_size) {
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

void error(char* msg, int lnum, int chnum) {
  printf("ERROR: %s\n%s: line: %d, column: %d.\n", msg, filename, lnum, chnum);
  exit(1);
}
