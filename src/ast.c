/*
 *
 *
 * The AST is built by the parser, but this code file is meant to be
 * responsible to navigate and manipulate the AST after (but also during)
 * the parsing.
 *
 */

#include <string.h>
#include "cli.h"
#include "ast.h"

void graphvizAstRec(Node* node);

Node* astFirstLeaf(Node* ast) {
  Node* firstChild = ast;

  while(1) {
    if(firstChild->nChildren > 0) firstChild = firstChild->children[0];
    else break;
  }
  return firstChild;
}

Node* astLastLeaf(Node* ast) {
  Node* lastChild = ast;

  while(1) {
    if(lastChild->nChildren > 0) {
      lastChild = lastChild->children[lastChild->nChildren - 1];
    } else break;
  }
  return lastChild;
}

int whichChild(Node* node) {
  if(!node->parent) return 0;

  for(int i = 0; i < node->parent->nChildren; i++) {
    if(node->id == node->parent->children[i]->id) return i;
  }

  genericError("AST node with corrupted ID.");
}

void postorderTraverse(Node* node, void (*visit)(Node*)) {
  for(int i = 0; i < node->nChildren; i++) {
    if(!node->children[i]) 
      genericError("Internal bug: AST node with NULL child.");
    postorderTraverse(node->children[i], visit);
  }

  visit(node);
}

void graphvizAst(Node* ast) {
  if(cli.outputType != OUT_GRAPHVIZ) return;

//  printf("digraph G {\n");
  printf("digraph G%d {\n", ast->id);
  graphvizAstRec(ast);
  printf("}");
  printf("\n");
}

void graphvizAstRec(Node* node) {
  char* format = "%s";
  int len = strlen(format) + MAX_NODE_NAME;
  char nodeName[len];
  strReplaceNodeAbbrev(nodeName, format, node); 

  if(node->type == NTTerminal && node->token->type == TTLitString)
    printf("%d [label=%s];\n", node->id, nodeName);
  else 
    printf("%d [label=\"%s\"];\n", node->id, nodeName);

  for(int i = 0; i < node->nChildren; i++) {
    printf("%d -> %d;\n", node->id, node->children[i]->id);
    graphvizAstRec(node->children[i]);
  }
}

void checkTree(Node* node, int nodeCount) {
  int id = node->id;
  int type = node->type;
  int nChildren = node->nChildren;
  Node* parent = node->parent;

  if(node->type != NTProgram && !node->parent) {
printf("type: %d, id: %d\n", node->type, node->id);
    genericError("Internal error: non-root AST node without parent.");
  }

  if(id < 0 || id > nodeCount)
    genericError("Internal memory error.");

//  if(cli.outputType == OUT_DEBUG) {
//    printf("ID: %d, type: %d, nch: %d\n", node->id, 
//      node->type, node->nChildren);
//  }

  for(int i = 0; i < node->nChildren; i++) {
    Node* chnode = node->children[i];
//    if(cli.outputType == OUT_DEBUG) {
//      printf("ch[%d]: ID: %d, type: %d, nch: %d\n", i, chnode->id, 
//        chnode->type, chnode->nChildren);
//    }
  }
  for(int i = 0; i < node->nChildren; i++) 
    checkTree(node->children[i], nodeCount);
}

