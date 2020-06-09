
#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "datast.h"

#define N_GPR 7

typedef struct stCodegenState{
  FILE* file;
  char* filename;
  char* busyRegisters;  // which registers are free
  char nGPR;  // how many general purpose registers (GPR)
  char** nameGPR;  // names of the GPRs
  char* code;  // code generated for the current file
  int nLabels;
} CodegenState;

typedef enum enInstructionType {
  INS_LABEL,
  INS_GLOBAL,
  INS_SYSCALL,
  INS_SECTION,
  INS_MOV,
  INS_ADD,
  INS_SUB,
  INS_NOP,
  INS_INC,
  INS_DEC,
  INS_NEG,
  INS_NOT,
  INS_RET,
  INS_CALL,
  INS_AND,
  INS_OR,
  INS_MUL,
  INS_IMUL,
  INS_CMP,
  INS_JMP,
  INS_JZ,
  INS_JNZ,
  INS_JG,
  INS_JGE,
  INS_JE,
  INS_JNE,
  INS_JL,
  INS_JLE
} InstructionType;

CodegenState codegenState;

void codegenStart(FILE* file, char* filename, Node* ast);
void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2);
void appendNodeCode(Node* node, char* text);
void declareGlobalVar(Node* node, char* varName, char size);
char* getSymbolRef(Symbol* sym);
char* getSymbolSizeRef(Symbol* sym);
void initializeRegisters();

#endif

