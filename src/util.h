/*
 *
 *
 * Utilitary functions used across several phases of the compilation to deal 
 * with things such as messaging, string formatting, etc.
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include "datast.h"

// Colored messages in the terminal
#define ERROR_COLOR_START "\033[1;31m"
#define WARN_COLOR_START "\033[33m"
#define COLOR_END "\033[m"

/*
 * Prints the line where a character is located and highlights the character. 
 *
 * file: pointer to the source file (should be open, will be rewinded and
 *   will not be closed).
 * filename: name of the source file being processed.
 * lnum: line number where the character appears.
 * chnum: column/position in the line where the character appears.
 *
 */
void printCharInFile(FILE* file, char* filename, int lnum, int chnum);

/*
 * Prints the line where the token is located and highlights the token. 
 *
 * file: pointer to the source file (should be open, will be rewinded and
 *   will not be closed).
 * filename: name of the source file.
 * token: the token to be printed.
 *
 */
void printTokenInFile(FILE* file, char* filename, Token token);

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

void genericError(char* msg);

void strReplaceTokenName(char* str, char* format, TokenType ttype);

void strReplaceNodeName(char* str, char* format, NodeType type);

void strReplaceNodeAndTokenName(char* str, char* format, Node* node);

void strReplaceNodeAbbrev(char* str, char* format, Node* node);

#endif

