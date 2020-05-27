/*
 *
 * Main file for the lexer of the compiler. The specification of the
 * valid language tokens is at docs/lexicon.txt.
 *
 */

#include <string.h>
#include "util.h"
#include "lexer.h"

/*
 * Reads the next character of the source file and manages the related
 * lexer state variables.
 *
 * returns: the character read or EOF.
 *
 */
char lexerGetChar();

/*
 * Adds a new token to the list of processed tokens. 
 *
 * size: the amount of characters (bytes) in this token.
 * type: type of token (identifier, integer literal, AND operator, etc.)
 * lnum: line number where this token was found.
 * chnum: position/column number where the token was found in the line.
 *
 */
void addToken(int size, TokenType type, int lnum, int chnum);

/*
 * Processes identifiers and keywords.
 *
 */
void eatIDKW();

/*
 * Processes integers and float number literals.
 *
 */
void eatNumber();

/*
 * Processes comments and the division operator. 
 *
 */
void eatSlash(); 

/*
 * Processes tokens constituted of a single character, like (, ), {, }, etc.
 *
 *
 * Note: Need to improve string eating. Only valid ASCII or UTF-8 
 * strings should be allowed.
 *
 */
void eatDQuote();

/*
 * Processes tokens constituted of a single character, like (, ), {, }, etc.
 *
 */
void eatSingleSymb();

/*
 * Processes operator tokens constituted of two characters,
 * such as ==, +=, -=, ++, --.
 *
 */
void eatDoubleSymb();

// Size of the buffer used to read characters
#define BUFFER_SIZE 100000

// Used to decide how much memory to allocate for tokens, at first.
#define INITIAL_MAX_TOKENS 250

// To show debug messages:
#define DEBUG


void lexerStart(FILE* sourcefile, char* sourcefilename) {

// Prints the source file
#ifdef DEBUG
//  printFile(sourcefile);
#endif

  // Buffer to read the chars, is reset at the end of each token
  char charBuffer[BUFFER_SIZE];

  // Holds global state variables of the lexer
  lexerState = (LexerState) {
    .maxTokens = INITIAL_MAX_TOKENS,
    .buffer = charBuffer,
    .file = sourcefile,
    .filename = sourcefilename,
    .lastChar = '\0',
    .lnum = 1,
    .chnum = 1,
    .nTokens = 0, // number of tokens processed so far
    .tokens = NULL
  };

  // The list of tokens in the source file
  lexerState.tokens = (Token*) malloc(INITIAL_MAX_TOKENS * sizeof(Token));

  // starts with the first character
  if(!feof(sourcefile)) {
    charBuffer[0] = lexerGetChar();
  }

  while(!feof(sourcefile)) {
    char ch = charBuffer[0];

    // depending on the character read, call a method to process next token(s)
    // All functions should finish with the next character in buffer[0],
    // and lexerState.chnum as the position (column) of buffer[0]
    if(ch == '/') eatSlash();
    else if(ch == '"') eatDQuote();
    else if(isSingleCharToken(ch)) eatSingleSymb();
    else if(startsDoubleOp(ch)) eatDoubleSymb();
    else if(isAlpha(ch) || ch == '_') eatIDKW();
    else if(isNum(ch)) eatNumber();
    else if(isWhitespace(ch)) charBuffer[0] = lexerGetChar();
    else { // unexpected character
      char* format = "Unexpected character: '%c'.";
      int len = strlen(format) - 1;
      char str[len];
      sprintf(str, format, ch); 
      lexError(str);
    }
  }
}

void eatIDKW()
{
  int bufpos = 1;
  char* buffer = lexerState.buffer;
  int lnum = lexerState.lnum;
  int chnum = lexerState.chnum;
  char ch = lexerGetChar();
  TokenType type = TTId;

  while(isNum(ch) || isAlpha(ch) || ch == '_') { // Still ID or keyword
    buffer[bufpos] = ch;
    ch = lexerGetChar();
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
  addToken(bufpos, type, lnum, chnum);
  buffer[0] = lexerState.lastChar;
  return;
}

void eatNumber()
{
  int bufpos = 1;
  char* buffer = lexerState.buffer;
  int lnum = lexerState.lnum;
  int chnum = lexerState.chnum;
  char ch = lexerGetChar();
  TokenType type = TTLitInt;

  while(isNum(ch)) {
    buffer[bufpos] = ch;
    ch = lexerGetChar();
    bufpos++;
  }

  if(ch == '.') { // float, read the rest of the number
    buffer[bufpos] = ch;
    ch = lexerGetChar();
    bufpos++;

    if(isNum(ch)) {
      type = TTLitFloat;

      while(isNum(ch)) {
        buffer[bufpos] = ch;
        ch = lexerGetChar();
        bufpos++;
      }

    } else {
      lexError("Invalid number.");
    }
  }

  addToken(bufpos, type, lnum, chnum);
  buffer[0] = lexerState.lastChar;
  return;
}

void eatDoubleSymb()
{
  int bufpos = 1;
  char* buffer = lexerState.buffer;
  int lnum = lexerState.lnum;
  int chnum = lexerState.chnum;

  char ch = lexerGetChar();
  TokenType type = TTUnknown;
  int numChars = 2;

  if(buffer[0] == '+') {
    if(ch == '+') {
      type = TTIncr;
    } else if(ch == '=') {
      type = TTAdd;
    } else { // just a plus
      type = TTPlus;
      numChars = 1;
    }
  } else if(buffer[0] == '-') {
    if(ch == '-') {
      type = TTDecr;
    } else if(ch == '=') {
      type = TTSub;
    } else { // just a minus
      type = TTMinus;
      numChars = 1;
    }
  } else if(buffer[0] == '=') {
    if(ch == '=') {
      type = TTEq;
    } else if(ch == '>') {
      type = TTArrow;
    } else { // just an assignment
      type = TTAssign;
      numChars = 1;
    }
  } else if(buffer[0] == '>') {
    if(ch == '=') {
      type = TTGEq;
    } else { // just a greater than
      type = TTGreater;
      numChars = 1;
    }
  } else if(buffer[0] == '<') {
    if(ch == '=') {
      type = TTLEq;
    } else { // just a less than
      type = TTLess;
      numChars = 1;
    }
  }

  if(numChars == 2) {
    buffer[bufpos] = ch;
    addToken(numChars, type, lnum, chnum);
    buffer[0] = lexerGetChar();
  } else {
    addToken(numChars, type, lnum, chnum);
    buffer[0] = ch;
  }

  return;
}

void eatSingleSymb()
{
  char* buffer = lexerState.buffer;
  int lnum = lexerState.lnum;
  int chnum = lexerState.chnum;
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
    case ',': type = TTComma;
      break;
  }

  addToken(1, type, lnum, chnum);
  buffer[0] = lexerGetChar();
  return;
}

void eatDQuote()
{
  int bufpos = 1;
  char* buffer = lexerState.buffer;
  int lnum = lexerState.lnum;
  int chnum = lexerState.chnum;
  char ch = lexerGetChar();

  while(ch != '\n' && ch != '"') { // read string till end
    buffer[bufpos] = ch;
    ch = lexerGetChar();
    bufpos++;
  }

  if(ch == '\n') lexError("Line break in the middle of string.");

  // Here ch == '"'
  buffer[bufpos] = ch;
  addToken(bufpos + 1, TTLitString, lnum, chnum);
  buffer[0] = lexerGetChar();
  return;
}

void eatSlash()
{
  char* buffer = lexerState.buffer;
  int lnum = lexerState.lnum;
  int chnum = lexerState.chnum;
  char ch = lexerGetChar();

  if(ch == '/') { // it was a comment
    lexerGetChar();

    // discard until newline
    while(lexerState.lastChar != '\n') lexerGetChar();
    buffer[0] = lexerGetChar();
  } else { // not a comment, thus division
    addToken(1, TTDiv, lnum, chnum); // adds division op.
    buffer[0] = ch;
  }
  return;
}

void addToken(int size, TokenType type, int lnum, int chnum) {
  char* tokenName = (char*) malloc((size + 1) * sizeof(char));
  Token token = { tokenName, size, type, lnum, chnum };
  strncpy(tokenName, lexerState.buffer, size);
  token.name[size] = '\0';

  if(lexerState.nTokens >= lexerState.maxTokens) {
    // doubles the size of the array of tokens
    lexerState.tokens = (Token*) realloc(
      lexerState.tokens, sizeof(Token) * lexerState.maxTokens * 2);

    lexerState.maxTokens *= 2;
  }
  lexerState.tokens[lexerState.nTokens] = token;
  lexerState.nTokens++;
}

char lexerGetChar() {
  if(!feof(lexerState.file)) {
    char ch = fgetc(lexerState.file);
    lexerState.lastChar = ch;
    lexerState.prevLnum = lexerState.lnum;
    lexerState.prevChnum = lexerState.chnum;

    if(ch == '\n') {
      lexerState.lnum++;
      lexerState.chnum = 0;
    } else {
      lexerState.chnum++;
    }
    return ch;
  }
  else return EOF;
}

