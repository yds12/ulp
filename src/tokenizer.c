#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

#define BUFFER_SIZE 100000
#define TOKENS_SIZE 1000

char* filename;

void error(char* msg, int lnum, int chnum);

void addToken(char * buffer, int size, TokenType type, int lnum, int chnum);

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

void tokenizer_start(FILE* sourcefile, char* sourcefilename) {
  filename = sourcefilename;

  // The list of tokens in the source file
  tokens = (Token*) malloc(TOKENS_SIZE * sizeof(Token));

  // Buffer to read the chars, is reset at the end of each token
  char char_buffer[BUFFER_SIZE];
  int i = 0;
  n_tokens = 0;
  int lnum = 1; // line number
  int chnum = 1; // char/column number
  char ch;

  while(!feof(sourcefile)) {
    ch = fgetc(sourcefile);
    char_buffer[i] = ch;

    if(ch == '\n') {
      lnum++; // increments line
      chnum = 1;
    } else chnum++;

    if(ch == '/') { // beginning of a comment or division
      processSlash(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(ch == '"') { // beginning of a string
      processDQuote(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
      ch == ';' || ch == '*' || ch == '%' || ch == ':') 
    {
      processSingleSymb(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(ch == '=' || ch == '+' || ch == '-') {
      processDoubleSymb(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(is_alpha(ch) || ch == '_') { // ID or keyword
      processIDKW(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(is_num(ch)) { // number
      processNumber(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(is_whitespace(ch)) {
      if(i > 0) { // end of a token
        addToken(char_buffer, i, TTUnknown, lnum, chnum);
      }
      i = 0; // restart buffer
    } else i++;

    if(i == BUFFER_SIZE) {
      error("File too long. Buffer overflowed.", lnum, chnum);
    }
  }

  for(int i = 0; i < n_tokens; i++) {
    printf("tt_%d @%d,%d, len: %d, ||%s||\n", tokens[i].type, 
      tokens[i].lnum, tokens[i].chnum, tokens[i].name_size, tokens[i].name);
  }
  printf("Total tokens: %d\n", n_tokens);
}

void processIDKW(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown, *lnum, *chnum - *bufpos);
    buffer[0] = buffer[*bufpos];
  }
  *bufpos = 1;

  char ch = fgetc(sourcefile);
  (*chnum)++;

  TokenType type = TTId;

  while(is_num(ch) || is_alpha(ch) || ch == '_') { // Still ID or keyword
    buffer[*bufpos] = ch;
    ch = fgetc(sourcefile);
    (*bufpos)++;
    (*chnum)++;
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
  addToken(buffer, *bufpos, type, *lnum, *chnum);

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
  printf("A number started\n");
}

void processDoubleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown, *lnum, *chnum - *bufpos);
    buffer[0] = buffer[*bufpos];
  }
  *bufpos = 1;

  char ch = fgetc(sourcefile);
  (*chnum)++;

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
    addToken(buffer, 2, TTEq, *lnum, *chnum);
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

    addToken(buffer, 1, TTAssign, *lnum, *chnum);
    *bufpos = 0;
    buffer[*bufpos] = ch;
  }
}

void processSingleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown, *lnum, *chnum - *bufpos);
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

  addToken(buffer, *bufpos, type, *lnum, *chnum - 1);
  *bufpos = 0;
}

// Need to improve string processing. Only valid ASCII or UTF-8 strings should
// be allowed
void processDQuote(FILE* sourcefile, char* buffer, int* bufpos,
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, TTUnknown, *lnum, *chnum - *bufpos);
    buffer[0] = buffer[*bufpos];
    *bufpos = 1;
  }

  char ch = fgetc(sourcefile);
  while(ch != '\n' && ch != '"') { // read string till end
    buffer[*bufpos] = ch;
    ch = fgetc(sourcefile);
    (*bufpos)++;
    (*chnum)++;
  }

  if(ch == '\n') { // error: broke line in the middle of string
    (*lnum)++;
    *chnum = 1;
    error("Line break in the middle of string.", *lnum, *chnum);
  } else if(ch == '"') {
    addToken(buffer, *bufpos, TTLitString, *lnum, *chnum);
    *bufpos = 0;
  }
}

void processSlash(FILE* sourcefile, char* buffer, int* bufpos,
  int* lnum, int* chnum)
{
  char ch = fgetc(sourcefile);

  if(ch == '/') { // it was a comment
    ch = fgetc(sourcefile);
    while(ch != '\n') ch = fgetc(sourcefile); // discard until newline
    (*lnum)++;
    *chnum = 1;
  } else if(*bufpos > 0) { // not a comment, thus division
    addToken(buffer, *bufpos, TTUnknown, *lnum, *chnum); // adds previous token
    addToken(buffer + (*bufpos), 1, TTDiv, *lnum, *chnum); // adds division op.
  }
  *bufpos = 0; // resets buffer
  return;
}

void addToken(char * buffer, int size, TokenType type, int lnum, int chnum) {
  if(size == 0) return;
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

void error(char* msg, int lnum, int chnum) {
  printf("ERROR: %s\n%s: line: %d, column: %d.\n", msg, filename, lnum, chnum);
  exit(1);
}
