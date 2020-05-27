/*
 * Header file for the lexer and lexer related functions, structs, enums
 * and global variables.
 */

#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include "datast.h"

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
int isSingleCharToken(char character);

/*
 * Checks whether a character could be the first in a 2-char operator, such
 * as ==, +=, ++, -= or --. 
 *
 * character: the character to be checked.
 * returns: 1 if it is, 0 otherwise.
 *
 */
int startsDoubleOp(char character);

#endif

