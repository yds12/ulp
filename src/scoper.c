
#include "scoper.h"
#include "util.h"
#include "ast.h"
#include "cli.h"

void addSymbol(Node* scopeNode, Token* token, SymbolType type);
void lookupSymbol(Node* scopeNode, Symbol* symbol);
void printScopeNodes(Node* node);

// Only the program and statement blocks have scope
int bearsScope(Node* node);

void scopeCheckerStart(FILE* file, char* filename, Node* ast) {
  if(cli.outputType <= OUT_DEBUG)
    printf("Starting scope checking...\n");

  postorderTraverse(ast, &printScopeNodes);
}

void addSymbol(Node* scopeNode, Token* token, SymbolType type) {
}

void lookupSymbol(Node* scopeNode, Symbol* symbol) {
}

void printScopeNodes(Node* node) {
  if(cli.outputType > OUT_DEBUG) return;

  if(bearsScope(node))
    printf("Node ID %d type %d has scope.\n", node->id, node->type);
  else
    printf("Node ID %d type %d has NO scope.\n", node->id, node->type);
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

