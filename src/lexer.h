/*
 * Header file for the lexer and lexer related functions, structs, enums
 * and global variables.
 */

#ifndef LEXER_H
#define LEXER_H

// Represents a token.
typedef struct stToken {
  char * name;
  int nameSize;
  short type;
  int lnum;
  int chnum;
} Token;

// Represents the global state of the lexer.
typedef struct stLexerState {
  int maxTokens; // current size of the array of tokens
  char* buffer;
  FILE* file;
  char lastChar;
  int lnum;      // this should always point to the line of the last char read
  int chnum;     // this should always point to the position of the last char
  int prevLnum;
  int prevChnum;
} LexerState;

/*
 * Global lexer variables.
 */

// file being manipulated.
char* filename;

// Global state of the lexer.
LexerState lexerState;

// List of tokens processed.
Token* tokens;

// Number of tokens processed.
int n_tokens;

// Represents the types of tokens allowed in the language.
typedef enum enTokenType {
  TTUnknown,
  TTId,

  // literals
  TTLitInt,
  TTLitString,
  TTLitFloat,
  TTLitBool,
  TTLitArray,

  // structural
  TTLPar,
  TTRPar,
  TTLBrace,
  TTRBrace,
  TTSemi,
  TTColon,

  // operators
  TTDiv,
  TTPlus,
  TTMinus,
  TTMod,
  TTMult,
  TTAssign,
  TTEq,
  TTIncr,
  TTDecr,

  // keywords
  TTIf,
  TTElse,
  TTFor,
  TTFunc,
  TTWhile,
  TTNext,
  TTBreak,
  TTInt,
  TTString,
  TTBool,
  TTFloat,
  TTAnd,
  TTOr,
  TTNot
} TokenType;

/*
 * Starts the lexer.
 *
 * sourcefile: pointer to the source file to process (file should be already
 *   open for reading/text mode). The file will be closed at the end by this
 *   function.
 * sourcefilename: name of the source file, used for messages.
 *
 */
void lexerStart(FILE* sourcefile, char* filename);

/*
 * Prints a text file. 
 *
 * file: pointer to the file (should be open, will be rewinded and
 *   will not be closed).
 *
 */
void printFile(FILE* file);

/*
 * Prints the line where the token is located and highlights the token. 
 *
 * file: pointer to the source file (should be open, will be rewinded and
 *   will not be closed).
 * token: the token to be printed.
 *
 */
void printTokenInFile(FILE* file, Token token);

/*
 * Prints the line where a character is located and highlights the character. 
 *
 * file: pointer to the source file (should be open, will be rewinded and
 *   will not be closed).
 * lnum: line number where the character appears.
 * chnum: column/position in the line where the character appears.
 *
 */
void printCharInFile(FILE* file, int lnum, int chnum);

/*
 * Prints an error message and exits the program with exit code 1. 
 *
 * msg: message to be printed. 
 *
 */
void error(char* msg);

/*
 * Checks whether a character is whitespace. 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isWhitespace(char character);

/*
 * Checks whether a character is a letter (ASCII only). 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isAlpha(char character);

/*
 * Checks whether a character is a number. 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isNum(char character);

/*
 * Checks whether a character is one of the single character tokens, such
 * as (, ), %, ;, etc. 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isSingleCharOp(char character);

/*
 * Checks whether a character could be part of a "double" operator, such
 * as ==, ++ or --. 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int belongsToDoubleOp(char character);

#endif

