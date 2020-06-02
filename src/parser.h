/*
 * Header file for the parser and parser related functions, structs, enums
 * and global variables.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "datast.h"

// The parser is a LR(1) parser, and it uses a stack of subtrees that can
// be reduced into larger subtrees when a production rule is matched. This
// structure represents the stack.
typedef struct stParserStack {
  Node** nodes;
  int pointer;
  int maxSize;
} ParserStack;

// Global state of the parser
typedef struct stParserState {
  FILE* file;
  char* filename;
  int nextToken;
  int nTokens;  // from lexerState
  Token** tokens;  // from lexerState
  int maxNodes;  // current size of the list of nodes
  int nodeCount;  // number of nodes created so far
  Node* ast;  // the final AST
} ParserState;

// Global state of the parser
ParserState parserState;

// The stack of subtrees of the LR parser
ParserStack pStack;

// The list of pointers to the nodes that comprise the trees
Node** pNodes;

/*
 * Starts the parser.
 *
 * file: pointer to the source file to process (file should be already
 *   open for reading/text mode). The file will be left open.
 * filename: name of the source file, used for messages.
 * nTokens: number of tokens processed by the lexer.
 * tokens: the array of tokens (pointers).
 *
 */
void parserStart(FILE* file, char* filename, int nTokens, Token** tokens);

/*
 * The parser is a LR(1) parser, and it uses a stack of subtrees that can
 * be reduced into larger subtrees when a production rule is matched. This
 * function initializes the structures that are used by the stack.
 *
 */
void initializeStack();

/*
 * The AST nodes are all stored in a dynamic list. This function allocates
 * memory for a new node, initializes it and manages the nodes list if
 * necessary.
 *
 * type: the type of node (expression, statement, etc.).
 * returns: a pointer to the newly created node.
 *
 */
Node* newNode(NodeType type);

/*
 * Helper function to facilitate node creation and addition to the stack.
 * It allocates memory for the child nodes and set the links between parent
 * and children.
 *
 * type: the type of node to be created.
 * nChildren: the number of children for the new node.
 * ...: a variable number of child nodes (the number must match nChildren).
 * returns: a pointer to the newly created node.
 *
 */
Node* createAndPush(NodeType type, int nChildren, ...);

/*
 * Pushes a node onto the stack. This should be done either when a new token
 * is read and turned into a terminal node, or when a number of nodes on
 * the top of the stack match a production rule and are reduced into a new 
 * node. This new node must be pushed into the stack.
 *
 * node: the node to be pushed.
 *
 */
void stackPush(Node* node);

/*
 * Pops n nodes from the top of the stack, and returns the last popped node.
 * When a number of nodes on the top of the stack match a production rule,
 * they must be popped and a new node (which possibly becomes parent of those)
 * must be pushed in their place.
 *
 * n: number of nodes to be popped.
 * returns: the last node popped.
 *
 */
Node* stackPop(int n);

/*
 * Allocates memory for the children of a node.
 *
 * node: the parent node.
 * nChildren: the number of children that this node will have.
 *
 */
void allocChildren(Node* node, int nChildren);

/*
 * Looks at the next token in queue of unprocessed tokens and returns it
 * (without removing it from the queue). This is the 1 in the LR(1) expression:
 * we are allowed to peek 1 token ahead to help us decide which rule to apply.
 *
 * returns: the next unprocessed token.
 *
 */
Token lookAhead();

/*
 * Gets a node at a specified offset from the top of the stack without 
 * popping it.
 *
 * offset: 0 to get the node on the top of the stack, 1 for next, and so on...
 * returns: the node or NULL if the offset is not valid.
 *
 */
Node* fromStackSafe(int offset);

/*
 * Checks whether the node type is a type of statement (such as an if 
 * statement, a while statement, a assignment statement, etc.).
 *
 * type: the node type to be checked. 
 * returns: 1 if it is a type of statement, 0 otherwise.
 *
 */
int isSubStatement(NodeType type);

/*
 * Checks whether a token type is a assignment operator type (=, +=, -=, etc.). 
 *
 * type: the token type to be checked. 
 * returns: 1 if it is a assignment operator type, 0 otherwise.
 *
 */
int isAssignmentOp(TokenType type);

/*
 * Checks whether the node is a terminal node and that its token has the 
 * specified type. If not, displays a syntax error and terminates the program.
 *
 * node: the node that will be checked.
 * type: the type that the token of this node must have.
 * msg: an error message to be displayed if the node's token does not have 
 *   the correct type. This message will be preppended to a standard message 
 *   saying that a type was expect but another found.
 *
 */
void assertTokenEqual(Node* node, TokenType ttype, char* msg);

/*
 * Checks whether the node has the specified type, if not displays a
 * syntax error and terminates the program.
 *
 * node: the node that will be checked.
 * type: the type that this node must have.
 * msg: an error message to be displayed if the node does not have the correct
 *   type. This message will be preppended to a standard message saying that
 *   a type was expect but another found.
 *
 */
void assertEqual(Node* node, NodeType type, char* msg);

/*
 * Outputs a syntax error message and terminates the program. 
 *
 * format: a format string for the error message containing the error message
 *   and one %s that will be replaced by the node/token name.
 * node: the node that caused the error.
 * leafNode: a leaf from the error-causing node, which contains a token that
 * will be used to get the line and column number.
 *
 */
void parsErrorHelper(char* format, Node* node, Node* leafNode);

/*
 * Outputs a syntax error message and terminates the program. 
 *
 * msg: error message.
 * lnum: line number where the error is found.
 * chnum: character/column number in the line where the error is found.
 *
 */
void parsError(char* msg, int lnum, int chnum);

/*
 * Debug function. Prints the current stack. 
 *
 *
 */
void printStack();

#endif

