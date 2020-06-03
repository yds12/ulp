
#include <string.h>  // remove this from here if helper functions are moved
#include <stdlib.h>  // remove this from here if helper functions are moved
#include "scoper.h"
#include "util.h"
#include "ast.h"
#include "cli.h"

// Initial size of a symbol table
#define MAX_INITIAL_SYMBOLS 10

/*
 * Tries to add a symbol to the symbol table of the nearest scope for the
 * specified node.
 *
 * node: the node where to start the search for the symbol.
 * token: the token containing the name of the symbol.
 * type: type of symbol to be added.
 * argNum: if it is a function argument, the position of the argument (starting
 *   from 0).
 *
 */
void tryAddSymbol(Node* node, Token* token, SymbolType type, short argNum);

/*
 * Does all the scope checking for a node. This includes adding declared
 * symbols, checking for redeclarations and use of undeclared symbols.
 *
 * node: the node to be checked.
 *
 */
void resolveScope(Node* node);

/*
 * Finds and returns a symbol from a scope-bearing node (i.e. looks only in
 * this node, does not search upwards).
 *
 * scopeNode: a scope-bearing node (that has a symbol table) wherein we want
 *   to find a symbol.
 * symToken: a token containing the name of the symbol.
 * returns: the symbol, or NULL if not found.
 *
 */
Symbol* findSymbol(Node* scopeNode, Token* symToken);

/*
 * Displays a scope error message and terminates the program with exit code 1
 * (error).
 *
 * msg: the message text. 
 * lnum: line number of the error.
 * chnum: character/column number in the line where the error is found.
 *
 */
void scoperError(char* msg, int lnum, int chnum);

/*
 * Finds the nearest scope-bearing node (a node with a symbol table) to a
 * specified node.
 *
 * node: the node where to start the search. 
 * returns: the closest ancestor node containg a symbol table.
 *
 */
Node* getImmediateScope(Node* node);

/*
 * Allocates memory for a symbol table for a node, and initializes its values.
 *
 * scopeNode: the scope-bearing node that will hold the symbol table. 
 * returns: a pointer to the symbol table.
 *
 */
SymbolTable* createSymTable(Node* scopeNode);

/*
 * Adds a symbol to the symbol table of a specified scope-bearing node,
 * managing memory space for the table as needed.
 *
 * scopeNode: the scope-bearing node whose symbol table will hold the 
 *   new symbol. 
 * symbol: the symbol to be added. 
 *
 */
void addSymbol(Node* scopeNode, Symbol symbol);

/*
 * Debug function: prints the symbol table of a node. 
 *
 * scopeNode: the node whose symbol table is to be printed. 
 *
 */
void printSymTable(Node* scopeNode);

/*
 * Traverses the AST and adds all function symbols to the global scope.
 * This must be done before any other scope checking is done. This allows for
 * functions to be called regardless of where they are defined.
 *
 * ast: the root node of the AST. 
 *
 */
void hoistFunctions(Node* ast);

/*
 * Returns whether the specified node is a scope-bearing node (i.e. has a
 * symbol table).
 *
 * node: node to be checked. 
 * returns: 1 if it is a scope-bearing node, 0 otherwise.
 *
 */
int bearsScope(Node* node);


void scopeCheckerStart(FILE* file, char* filename, Node* ast) {
  scoperState = (ScoperState) {
    .file = file,
    .filename = filename
  };

  if(cli.outputType <= OUT_DEBUG)
    printf("Starting scope checking...\n");

  hoistFunctions(ast);
  postorderTraverse(ast, &resolveScope);
}

void tryAddSymbol(Node* node, Token* token, SymbolType type, short argNum) {
  Symbol newSym = { 
    .token = token, 
    .type = type,
    .argNum = argNum
  }; 
  Symbol* oldSym = lookupSymbol(node, token);

  if(oldSym) {
    char* fmt = "Redeclaration of '%s'.";
    char msg[strlen(fmt) + token->nameSize];
    sprintf(msg, fmt, token->name);
    scoperError(msg, token->lnum, token->chnum);
  }
  Node* scopeNode = getImmediateScope(node);
  addSymbol(scopeNode, newSym);
}

SymbolTable* createSymTable(Node* scopeNode) {
  scopeNode->symTable = (SymbolTable*) malloc(sizeof(SymbolTable));
  scopeNode->symTable->nSymbols = 0;
  scopeNode->symTable->maxSize = MAX_INITIAL_SYMBOLS;
  scopeNode->symTable->symbols = (Symbol**) 
    malloc(sizeof(Symbol*) * scopeNode->symTable->maxSize);

  return scopeNode->symTable;
}

void addSymbol(Node* scopeNode, Symbol symbol) {
//printf("Adding %s to NT %d\n", symbol.token->name, scopeNode->type);
  SymbolTable* st = scopeNode->symTable;
  if(!st) st = createSymTable(scopeNode);

  if(st->nSymbols >= st->maxSize) {
    st->symbols = (Symbol**) realloc(
      st->symbols, sizeof(Symbol*) * st->maxSize * 2);

    st->maxSize *= 2;
  }

  Symbol* newSym = (Symbol*) malloc(sizeof(Symbol));
  *newSym = symbol;
  st->symbols[st->nSymbols] = newSym;
  st->nSymbols++;
}

Symbol* lookupSymbol(Node* node, Token* symToken) {
  Node* lookNode = node;

  while(1) {
//printf("Searching %s in NT %d\n", symbol->token->name, lookNode->type);
    printSymTable(lookNode);

    if(bearsScope(lookNode)) {
      Symbol* sym = findSymbol(lookNode, symToken);
      if(sym) return sym;
    }
    if(!lookNode->parent) return NULL;
    lookNode = lookNode->parent;
  }
}

Symbol* findSymbol(Node* scopeNode, Token* symToken) {
  if(!scopeNode->symTable) return NULL;  // node doesn't have a symbol table

  SymbolTable* st = scopeNode->symTable;
  if(st->nSymbols == 0) return NULL;

  for(int i = 0; i < st->nSymbols; i++) {
    Symbol* tabSymbol = st->symbols[i];
    if(tabSymbol->token->nameSize != symToken->nameSize) continue;
    if(strncmp(tabSymbol->token->name, symToken->name, 
         symToken->nameSize) == 0) return tabSymbol;
  }
  return NULL;
}

Node* getImmediateScope(Node* node) {
  if(bearsScope(node)) return node;
  if(!node->parent) {
    genericError("Compiler bug: AST node without scope.");
  }
  return getImmediateScope(node->parent);
}

void hoistFunctions(Node* ast) {
  for(int i = 0; i < ast->nChildren; i++) {
    Node* fNode = ast->children[i]->children[0];

    if(fNode->type == NTFunction) {
      Node* termNode = fNode->children[0]->children[0];
      tryAddSymbol(fNode, termNode->token, STFunction, -1);
    }
  }
}

void resolveScope(Node* node) {
  if(node->type == NTIdentifier) {
    if(!node->parent) 
      genericError("Compiler bug: AST node missing parent.");

    Node* scopeNode = node;
    Node* parent = node->parent;
    SymbolType stype;

    if(parent->type == NTExpression || parent->type == NTAssignment
       || parent->type == NTCallExpr || parent->type == NTCallSt) {
      // identifier in use  -- check if declared 
      Token* token = node->children[0]->token;
      Symbol* oldSym = lookupSymbol(node, token);

      if(!oldSym) { // undeclared
        char* fmt = "Use of undeclared variable or function '%s'.";
        char msg[strlen(fmt) + token->nameSize];
        sprintf(msg, fmt, token->name);
        
        scoperError(msg, token->lnum, token->chnum);
      } else {
        char isFunc = (parent->type == NTCallExpr || parent->type == NTCallSt);

        if(isFunc && oldSym->type != STFunction) {
          char* fmt = 
            "'%s' has previously been declared as a variable, not a function.";

          char msg[strlen(fmt) + token->nameSize];
          sprintf(msg, fmt, token->name);
          
          scoperError(msg, token->lnum, token->chnum);
        } else if(!isFunc && oldSym->type == STFunction) {
          char* fmt = "'%s' has been declared as a function, not a variable.";
          char msg[strlen(fmt) + token->nameSize];
          sprintf(msg, fmt, token->name);
          
          scoperError(msg, token->lnum, token->chnum);
        }
      }
    } else { // identifier in declaration
      short argNum = -1;

      if(parent->type == NTFunction) { // function name
        //stype = STFunction;
        return; // functions already hoisted
      } else if(parent->type == NTDeclaration) { // variable name
        Node* ppNode = parent->parent;

        if(!ppNode) 
          genericError("Compiler bug: AST node missing parent.");

        if(ppNode->type == NTProgramPart) { // global variable
          stype = STGlobal;
        } else if(ppNode->type == NTStatement) { // local variable
          stype = STLocal;
        }

      } else if(parent->type == NTArg) { // function argument
        stype = STArg;
        argNum = whichChild(parent);

        if(parent->parent->parent->nChildren < 3)
          genericError("Compiler bug: Function AST node missing statement.");

        scopeNode = parent->parent->parent->children[2];
      }

      if(node->nChildren < 1)
        genericError("Compiler bug: Identifier AST node without child.");

      tryAddSymbol(scopeNode, node->children[0]->token, stype, argNum);
    }
  }
}

int bearsScope(Node* node) {
  if(node->type == NTProgram) return 1;
  if(node->type == NTStatement && node->nChildren > 0) {
    for(int i = 0; i < node->nChildren; i++) {
      if(!node->children[i]) {
        genericError("Internal bug: NULL AST node child.");
      }
      if(node->children[i]->type != NTStatement) return 0;
    }
    return 1;
  }
  return 0;
}

void scoperError(char* msg, int lnum, int chnum) {
  if(cli.outputType > OUT_DEFAULT) exit(1);

  if(lnum > 0) {
    printf("\nScope " ERROR_COLOR_START "ERROR" COLOR_END ": %s\n", msg);
    printCharInFile(scoperState.file, scoperState.filename, lnum, chnum);
  } else {
    printf("\nScope " ERROR_COLOR_START "ERROR" COLOR_END 
      ": %s\n%s.\n", msg, scoperState.filename);
  }
  exit(1);
}

void printSymTable(Node* scopeNode) {
  if(cli.outputType > OUT_DEBUG) return;
  if(!scopeNode->symTable) {
    printf("NT %d: No symtable.\n", scopeNode->type); 
    return;
  }
//  printf("NT %d: symtable %p.\n", scopeNode->type, scopeNode->symTable);
  printf("NT %d: symtable has %d.", scopeNode->type, 
    scopeNode->symTable->nSymbols);
  if(scopeNode->symTable->nSymbols > 0) {
    for(int i = 0; i < scopeNode->symTable->nSymbols; i++)
      printf(" %s [T:%d]", scopeNode->symTable->symbols[i]->token->name,
        scopeNode->symTable->symbols[i]->type);
  }
  printf("\n");
}

