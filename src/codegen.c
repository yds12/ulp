
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "util.h"
#include "ast.h"
#include "cli.h"
#include "scoper.h"

// initial length for the code string of a node, not considering user 
// defined identifiers
#define INITIAL_CODE_SIZE 30

void initializeRegisters();
char getReg();
char* getRegName(char regNum);
void freeReg(char regNum);
void emitCode(Node* node);
void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2);
void createCgData(Node* node);
void printRegs();
void allocateReg(Node* node);
void printNodeCode(Node* node);

void codegenStart(FILE* file, char* filename, Node* ast) {
  codegenState = (CodegenState) {
    .file = file,
    .filename = filename
  };

  initializeRegisters();
  postorderTraverse(ast, &emitCode);
}

void emitCode(Node* node) {
  char* fmt = "%s";
  char str[MAX_NODE_NAME];
  strReplaceNodeAndTokenName(str, fmt, node);

  if(cli.outputType <= OUT_DEBUG)
    printf("generating code for %s, ID: %d\n", str, node->id);

  if(node->type == NTExpression) {
    if(node->nChildren == 1) {
      Node* litOrIdNode = node->children[0];
      Token* token = litOrIdNode->children[0]->token;

      if(node->children[0]->type == NTLiteral) {
        createCgData(node);
        allocateReg(node);
        appendInstruction(node, INS_MOV, 
          getRegName(node->cgData->reg), token->name);
      } else if(node->children[0]->type == NTIdentifier) {
        Symbol* sym = lookupSymbol(node, token);
      }
    }
  }

  if(cli.outputType <= OUT_DEBUG) printNodeCode(node);
}

void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2) {
  if(!node->cgData) createCgData(node);

  int iLen = strlen(op1) + strlen(op2);
  char* fmt;

  switch(inst) {
    case INS_MOV: 
      fmt = "mov %s, %s\n";
      iLen += strlen(fmt);
      node->cgData->code = (char*) malloc(sizeof(char) * iLen);
      sprintf(node->cgData->code, fmt, op1, op2); 
      break;
  }
}

void initializeRegisters() {
  const char gprSize = N_GPR;
  codegenState.nGPR = N_GPR;
  codegenState.busyRegisters = (char*) malloc(sizeof(char) * N_GPR);
  codegenState.nameGPR = (char**) malloc(sizeof(char*) * N_GPR);

  char* regNames[N_GPR] = { "rbx", "r10", "r11", "r12", "r13", "r14", "r15" };
  for(int i = 0; i < codegenState.nGPR; i++) {
    codegenState.busyRegisters[i] = 0;
    codegenState.nameGPR[i] = (char*) malloc(sizeof(char) * 5);
    strcpy(codegenState.nameGPR[i], regNames[i]);
  }
}

char* getRegName(char regNum) {
  return codegenState.nameGPR[regNum];
}

void allocateReg(Node* node) {
  char reg = getReg();
  node->cgData->reg = reg;
}

void freeNodeReg(Node* node) {
  if(node->cgData) {
    freeReg(node->cgData->reg);
  } else genericError("Compiler bug: freeing register of an AST node "
           "without allocated register.");
}

char getReg() {
  for(int i = 0; i < N_GPR; i++) {
    if(!codegenState.busyRegisters[i]) {
      codegenState.busyRegisters[i] = 1;
      if(cli.outputType <= OUT_DEBUG) printRegs();
      return i;
    }
  }

  genericError("Code generation failed: out of registers.");
}

void freeReg(char regNum) {
  if(regNum < N_GPR) codegenState.busyRegisters[regNum] = 0;
  else genericError("Code generation bug: freeing invalid register.");
}

void createCgData(Node* node) {
  if(!node->cgData) {
    node->cgData = (CgData*) malloc(sizeof(CgData));
    node->cgData->code = NULL;
  }
}

void printNodeCode(Node* node) {
  if(node->cgData) {
    printf("> %s", node->cgData->code);
  } 
  else printf("> \n");
}

void printRegs() {
  for(int i = 0; i < N_GPR; i++) {
    if(codegenState.busyRegisters[i]) printf("1");
    else printf("0");
  }
  printf("\n");
}

