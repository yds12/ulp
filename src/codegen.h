
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
  // pseudo-instructions
  INS_LABEL,
  INS_GLOBAL,
  INS_SYSCALL,
  INS_SECTION,
  INS_DIVISION,
  INS_GETQUOTIENT,
  INS_GETREMAINDER,
  INS_EXIT,
  INS_PROLOGUE,
  INS_PROLOGUE_STACK,
  INS_EPILOGUE,
//  INS_EPILOGUE_STACK,
  INS_SETRET,
  INS_GETRET,

  // regular instructions
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
  INS_XOR,
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
  INS_JLE,
  INS_PUSH,
  INS_POP
} InstructionType;

extern CodegenState codegenState;

void codegenStart(FILE* file, char* filename, Node* ast);
void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2);
void appendNodeCode(Node* node, char* text);
void declareGlobalVar(Node* node, char* varName, char size);
char* getSymbolRef(Symbol* sym, Node* node);
char* getSymbolSizeRef(Symbol* sym, Node* node);
char* getArgRegName(short argPos);
void initializeRegisters();

#endif

