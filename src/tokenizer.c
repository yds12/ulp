#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

#define BUFFER_SIZE 100000
#define TOKENS_SIZE 1000

#define TTYPE_UNKNOWN -1
#define TTYPE_ID 0
#define TTYPE_LIT_INT 1
#define TTYPE_LIT_FLOAT 2
#define TTYPE_LIT_STRING 3

char* filename;

void error(char* msg, int lnum, int chnum);

Token addToken(char * buffer, int size, int lnum, int chnum);

// for straightforward 1 character symbols: +, -, *, {, }, (, )
void processSingleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
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
      ch == ';' || ch == '+' || ch == '-' || ch == '*') 
    {
      processSingleSymb(sourcefile, char_buffer, &i, &lnum, &chnum);
    } else if(ch == ' ' || ch == '\n') {
      if(i > 0) { // end of a token
        addToken(char_buffer, i, lnum, chnum);
      }
      i = 0; // restart buffer
    } else i++;

    if(i == BUFFER_SIZE) {
      error("File too long. Buffer overflowed.", lnum, chnum);
    }
  }

  for(int i = 0; i < n_tokens; i++) {
    printf("Type: %d, Size: %d, Pos:%d,%d, Content:||%s||\n", tokens[i].type, 
      tokens[i].name_size, tokens[i].lnum, tokens[i].chnum, tokens[i].name);
  }
  printf("Total tokens: %d\n", n_tokens);
}

void processSingleSymb(FILE* sourcefile, char* buffer, int* bufpos, 
  int* lnum, int* chnum)
{
  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, *lnum, *chnum - *bufpos);
    buffer[0] = buffer[*bufpos];
    *bufpos = 1;
  }
  addToken(buffer, *bufpos, *lnum, *chnum - 1);
  *bufpos = 0;
}

void processDQuote(FILE* sourcefile, char* buffer, int* bufpos,
  int* lnum, int* chnum)
{
printf("processing... char||%c|| 1st||%c|| pos:%d\n", 
  buffer[*bufpos], buffer[0], *bufpos);

  if(*bufpos > 0) { // add previous token
    addToken(buffer, *bufpos, *lnum, *chnum - *bufpos);
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
printf("adding... char||%c|| 1st||%c|| pos:%d\n", 
  buffer[*bufpos], buffer[0], *bufpos);
    addToken(buffer, *bufpos, *lnum, *chnum);
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
    addToken(buffer, *bufpos, *lnum, *chnum); // adds previous token
    addToken(buffer + (*bufpos), 1, *lnum, *chnum); // adds division op.
  }
  *bufpos = 0; // resets buffer
  return;
}

Token addToken(char * buffer, int size, int lnum, int chnum) {
//  printf("Generating token:%.*s\n", size, buffer);

  char* tokenName = (char*) malloc((size + 1) * sizeof(char));
  Token token = { tokenName, size, TTYPE_UNKNOWN, lnum, chnum };
  strncpy(tokenName, buffer, size);
  token.name[size] = '\0';
  tokens[n_tokens] = token;
  n_tokens++;
}

void error(char* msg, int lnum, int chnum) {
  printf("ERROR: %s\n%s: line: %d, column: %d.\n", msg, filename, lnum, chnum);
  exit(1);
}
