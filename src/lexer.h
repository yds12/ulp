/*
 * Header file for the lexer and lexer related functions, structs, enums
 * and global variables.
 */

#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>

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
  char* filename;
  char lastChar;
  int lnum;      // this should always point to the line of the last char read
  int chnum;     // this should always point to the position of the last char
  int prevLnum;
  int prevChnum;
  int nTokens;  // number of tokens processed
  Token* tokens; // list of processed tokens
} LexerState;

// Global state of the lexer.
LexerState lexerState;

// Represents the types of tokens allowed in the language.
typedef enum enTokenType {
  TTUnknown,
  TTEof,
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
  TTComma,
  TTArrow,

  // operators
  TTDiv,
  TTPlus,
  TTMinus,
  TTMod,
  TTMult,
  TTGreater,
  TTGEq,
  TTLess,
  TTLEq,
  TTEq,
  TTAssign,
  TTIncr,
  TTDecr,
  TTAdd,
  TTSub,

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
 *   open for reading/text mode). The file will be left open.
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
void lexError(char* msg);

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
 * Checks whether a character could be the first in a 2-char operator, such
 * as ==, +=, ++, -= or --. 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int startsDoubleOp(char character);

/*
 * Checks whether a token type is a literal type. 
 *
 * type: the token type to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isLiteral(TokenType type);

/*
 * Checks whether a token type is a (data) type type. 
 *
 * type: the token type to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isType(TokenType type);

/*
 * Checks whether a token type is a binary operator. 
 *
 * type: the token type to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int isBinaryOp(TokenType type);

/*
 * Gets the precedence of an operator token type. 
 *
 * type: the token type to be checked.
 * returns: The operator precedence. Maximum precedence is 0, meaning that 
 *   such operations will be executed first. If not an operator, returns -1.
 *
 */
int precedence(TokenType type);

/*
 * Checks whether the token type signals the end of an expression. 
 *
 * type: the token type to be checked.
 * returns: 1 if it does, 0 otherwise.
 *
 */
int isExprTerminator(TokenType type);

#endif

