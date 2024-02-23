/*
 *
 *
 * This file contains the code for architecture-dependent (x86) code
 * generation, especially functions that generate x86 nasm code.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "util.h"
#include "scoper.h"

#define MAX_INSTRUCTION_LEN 300

void initializeRegisters() {
  const char gprSize = N_GPR;
  codegenState.nGPR = N_GPR;
  codegenState.busyRegisters = (char*) malloc(sizeof(char) * N_GPR);
  codegenState.nameGPR = (char**) malloc(sizeof(char*) * N_GPR);

  char* regNames[N_GPR] = {
    "ebx", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };

  for(int i = 0; i < codegenState.nGPR; i++) {
    codegenState.busyRegisters[i] = 0;
    codegenState.nameGPR[i] = (char*) malloc(sizeof(char) * 5);
    strcpy(codegenState.nameGPR[i], regNames[i]);
  }
}

char* getArgRegName(short argPos) {
  char* reg = (char*) malloc(sizeof(char) * 4);

  switch(argPos) {
    case 0: sprintf(reg, "edi"); break;
    case 1: sprintf(reg, "esi"); break;
    case 2: sprintf(reg, "edx"); break;
    case 3: sprintf(reg, "ecx"); break;
    case 4: sprintf(reg, "r8d"); break;
    case 5: sprintf(reg, "r9d"); break;
    default: genericError("Code generation error: too many parameters");
  }
  return reg;
}

char* getSymbolRef(Symbol* sym, Node* node) {
  // TODO: free strings returned by this function
  if(sym->type == STGlobal) {
    char* ref = (char*) malloc(sizeof(char) * (sym->token->nameSize + 10));
    sprintf(ref, "[rel %s]", sym->token->name);
    return ref;
  } else if(sym->type == STLocal) {
    Node* scopeNode = getImmediateScope(node);
    if(!scopeNode) genericError("Code generation bug: missing AST scope node");
    if(!scopeNode->symTable) {
      genericError("Code generation bug: AST scope node without symbol table");
    }

    int pos = scopeNode->symTable->nStackVarsAcc -
      scopeNode->symTable->nLocalVars + sym->pos;

    char* ref = (char*) malloc(sizeof(char) * (sym->token->nameSize + 10));
    sprintf(ref, "[rbp - %d]", pos * 4);  // TODO: fixed size 4
    return ref;
  } else if(sym->type == STArg) {
    int pos = sym->pos;
    char* ref = (char*) malloc(sizeof(char) * (sym->token->nameSize + 10));
    sprintf(ref, "[rbp - %d]", pos * 4);  // TODO: fixed size 4
    return ref;
  }
  return NULL;
}

char* getSymbolSizeRef(Symbol* sym, Node* node) {
  // TODO for now the size is fixed. Should be decided according to data type.
  char* ref = getSymbolRef(sym, node);
  if(!ref) return NULL;

  char* sizeRef = (char*) malloc(
    sizeof(char) * (strlen(ref) + strlen("dword ") + 2));

  sprintf(sizeRef, "dword %s", ref);
  free(ref);
  return sizeRef;
}

void declareGlobalVar(Node* node, char* varName, char size) {
  appendNodeCode(node, varName);
  appendNodeCode(node, ": ");

  switch(size) {
    case 1: appendNodeCode(node, "resb"); break;
    case 2: appendNodeCode(node, "resw"); break;
    case 4: appendNodeCode(node, "resd"); break;
    case 8: appendNodeCode(node, "resq"); break;
  }
  appendNodeCode(node, " 1\n");
}

void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2) {
//  if(!node->cgData) createCgData(node);

  char instructionStr[MAX_INSTRUCTION_LEN];
  char* fmt;

  switch(inst) {
    case INS_MOV:
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand for MOV.");
      fmt = "mov %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_ADD:
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand for ADD.");
      fmt = "add %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_SUB:
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand for SUB.");
      fmt = "sub %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_IMUL:
      if(!op1 || !op2) genericError(
          "Code generation bug: empty instruction operand for IMUL.");
      fmt = "imul %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_DIVISION:
      if(!op1 || !op2) genericError(
          "Code generation bug: empty instruction operand for DIVISION.");
      fmt = "mov eax, %s\ncdq\nidiv %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_GETQUOTIENT:
      if(!op1) genericError(
          "Code generation bug: empty instruction operand for GET QUOTIENT.");
      fmt = "mov %s, eax\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_GETREMAINDER:
      if(!op1) genericError(
          "Code generation bug: empty instruction operand for GET REMAINDER.");
      fmt = "mov %s, edx\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_EXIT:
      if(!op1) genericError(
          "Code generation bug: empty instruction operand for EXIT.");
      fmt = "mov eax, 60\nmov edi, %s\nsyscall\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_AND:
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand for AND.");
      fmt = "and %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_OR:
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand for OR.");
      fmt = "or %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_CMP:
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand for CMP.");
      fmt = "cmp %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2);
      break;
    case INS_INC:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for INC.");
      fmt = "inc %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_DEC:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for DEC.");
      fmt = "dec %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_NEG:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for NEG.");
      fmt = "neg %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_NOT:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for NOT.");
      fmt = "not %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JMP:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JMP.");
      fmt = "jmp %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JZ:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JZ.");
      fmt = "jz %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JNZ:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JNZ.");
      fmt = "jnz %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JE:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JE.");
      fmt = "je %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JNE:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JNE.");
      fmt = "jne %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JG:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JG.");
      fmt = "jg %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JGE:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JGE.");
      fmt = "jge %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JL:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JL.");
      fmt = "jl %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_JLE:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for JLE.");
      fmt = "jle %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_PUSH:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for PUSH.");
      fmt = "push %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_POP:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for POP.");
      fmt = "pop %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_CALL:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for CALL.");
      fmt = "call %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_GLOBAL:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for GLOBAL.");
      fmt = "global %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_LABEL:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for LABEL.");
      fmt = "%s:\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_SECTION:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for SECTION.");
      fmt = "section .%s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_SETRET:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for SET RETURN.");
      fmt = "mov eax, %s\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_GETRET:
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for GET RETURN.");
      fmt = "mov %s, eax\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_PROLOGUE_STACK: // prologue with stack space allocation
      if(!op1) genericError(
        "Code generation bug: empty instruction operand for PROLOGUE.");
      fmt = "push rbp\nmov rbp, rsp\nsub rsp, %s\n"
        "push rbx\npush r12\npush r13\npush r14\npush r15\n";
      sprintf(instructionStr, fmt, op1);
      break;
    case INS_PROLOGUE:
      strcpy(instructionStr, "push rbp\nmov rbp, rsp\n"
        "push rbx\npush r12\npush r13\npush r14\npush r15\n");
      break;
    case INS_EPILOGUE:
      strcpy(instructionStr, ".epilogue:\n"
        "pop r15\npop r14\npop r13\npop r12\npop rbx\n"
        "mov rsp, rbp\npop rbp\nret\n");
      break;
    case INS_NOP: strcpy(instructionStr, "nop\n"); break;
    case INS_SYSCALL: strcpy(instructionStr, "syscall\n"); break;
    case INS_RET: strcpy(instructionStr, "ret\n"); break;
  }

  appendNodeCode(node, instructionStr);
}

void appendNodeCode(Node* node, char* text) {
  if(strlen(node->cgData->code) + strlen(text) + 10
     > node->cgData->maxCode) {
    node->cgData->maxCode *= 2;
    node->cgData->maxCode += strlen(text);
    node->cgData->code = (char*) realloc(node->cgData->code,
      sizeof(char) * node->cgData->maxCode);
  }

  strcat(node->cgData->code, text);
}
