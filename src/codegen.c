
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

#define INITIAL_BSSSEC_LEN 30
#define INITIAL_TEXTSEC_LEN 30
#define MAX_INSTRUCTION_LEN 80

void initializeRegisters();
char getReg();
char* getRegName(char regNum);
void freeReg(char regNum);
void freeNodeReg(Node* node);
void emitCode(Node* node);
void createCgData(Node* node);
void allocateReg(Node* node);
void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2);
void appendSecBss(char* text);
void appendSecText(char* text);
void appendNodeCode(Node* node, char* text);
void declareGlobalVar(char* varName, char size);
char* getSymbolRef(Symbol* sym);
void initializeSections();
void finalizeSections();
void printRegs();
void printNodeCode(Node* node);
void printSections();

void codegenStart(FILE* file, char* filename, Node* ast) {
  codegenState = (CodegenState) {
    .file = file,
    .filename = filename
  };

  initializeRegisters();
  initializeSections();
  postorderTraverse(ast, &emitCode);
  finalizeSections();
  printSections();
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
        createCgData(node);
        Symbol* varSym = lookupSymbol(node, token);

        // load the variable in a register
        allocateReg(node);
        appendInstruction(node, INS_MOV,
          getRegName(node->cgData->reg),
          getSymbolRef(varSym));
      }
    } else if(node->nChildren == 3) {
      if(node->children[1]->type == NTBinaryOp) { // binary operation
        Token* opToken = node->children[1]->children[0]->token;
        if(opToken->type == TTPlus || opToken->type == TTMinus) {
          createCgData(node);

          if(!node->children[0]->cgData || !node->children[2]->cgData) {
            genericError("Code generation bug: AST node without code info.");
          }

          InstructionType iType = (opToken->type == TTPlus) ? INS_ADD : INS_SUB;

          node->cgData->reg = node->children[0]->cgData->reg;
          appendInstruction(node, iType,
            getRegName(node->children[0]->cgData->reg),
            getRegName(node->children[2]->cgData->reg));
          freeNodeReg(node->children[2]);
        }
      }
    }
  }
  else if(node->type == NTDeclaration) {
    if(node->nChildren == 3) { // declaration with assignment
      if(!node->children[2]->cgData)
        genericError("Code generation bug: AST node without code info.");

      if(node->children[1]->nChildren < 1)
        genericError("Code generation bug: AST identifier node without child.");

      if(!node->children[1]->children[0]->token)
        genericError("Code generation bug: AST node missing token.");

      if(node->children[2]->type != NTExpression)
        genericError("Code generation bug: expression expected.");

      Token* varToken = node->children[1]->children[0]->token;
      Symbol* varSym = lookupSymbol(node, varToken);

      createCgData(node);
      appendInstruction(node, INS_MOV,
        getSymbolRef(varSym),
        getRegName(node->children[2]->cgData->reg));
      freeNodeReg(node->children[2]);
    }
  }
  else if(node->type == NTAssignment) {
    if(node->nChildren == 3) { // declaration with assignment
      if(!node->children[2]->cgData)
        genericError("Code generation bug: AST node without code info.");

      if(node->children[0]->nChildren < 1)
        genericError("Code generation bug: AST identifier node without child.");

      if(!node->children[0]->children[0]->token)
        genericError("Code generation bug: AST node missing token.");

      if(node->children[2]->type != NTExpression)
        genericError("Code generation bug: expression expected.");

      Token* varToken = node->children[0]->children[0]->token;
      Symbol* varSym = lookupSymbol(node, varToken);
      createCgData(node);

      if(node->children[1]->token->type == TTAssign) {
        appendInstruction(node, INS_MOV,
          getSymbolRef(varSym),
          getRegName(node->children[2]->cgData->reg));
      }
      else if(node->children[1]->token->type == TTAdd) {
        allocateReg(node);
        appendInstruction(node, INS_MOV,
          getRegName(node->cgData->reg),
          getSymbolRef(varSym));
        appendInstruction(node, INS_ADD,
          getRegName(node->cgData->reg),
          getRegName(node->children[2]->cgData->reg));
        appendInstruction(node, INS_MOV,
          getSymbolRef(varSym),
          getRegName(node->cgData->reg));
        freeNodeReg(node);
      }
      else if(node->children[1]->token->type == TTSub) {
      }

      freeNodeReg(node->children[2]);
    }
  }
  else if(node->type == NTProgram && node->symTable) {
    for(int i = 0; i < node->symTable->nSymbols; i++) {
      if(node->symTable->symbols[i]->type == STGlobal) {
        declareGlobalVar(node->symTable->symbols[i]->token->name, 4);
      }
    }
  }

  printNodeCode(node);
}

void declareGlobalVar(char* varName, char size) {
  appendSecBss(varName);
  appendSecBss(": ");

  switch(size) {
    case 1: appendSecBss("resb"); break;
    case 2: appendSecBss("resw"); break;
    case 4: appendSecBss("resd"); break;
    case 8: appendSecBss("resq"); break;
  }
  appendSecBss(" 1\n");
}

void appendInstruction(Node* node, InstructionType inst, char* op1, char* op2) {
  if(!node->cgData) createCgData(node);

  if(!op1 || !op2)
    genericError("Code generation bug: empty instruction operand.");

  char instructionStr[MAX_INSTRUCTION_LEN];
  char* fmt;

  switch(inst) {
    case INS_MOV: 
      fmt = "mov %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2); 
      break;
    case INS_ADD: 
      fmt = "add %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2); 
      break;
    case INS_SUB: 
      fmt = "sub %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2); 
      break;
  }

  appendNodeCode(node, instructionStr);
}

void appendSecBss(char* text) {
  if(strlen(codegenState.secBssCode) + strlen(text) + 10
     > codegenState.maxLenBss) {
    codegenState.secBssCode = (char*) realloc(codegenState.secBssCode, 
      sizeof(char) * codegenState.maxLenBss * 2);
    codegenState.maxLenBss *= 2;
  }

  strcat(codegenState.secBssCode, text);
}

void appendSecText(char* text) {
  if(strlen(codegenState.secTextCode) + strlen(text) + 10
     > codegenState.maxLenText) {
    codegenState.secTextCode = (char*) realloc(codegenState.secTextCode,
      sizeof(char) * codegenState.maxLenText * 2);
    codegenState.maxLenText *= 2;
  }

  strcat(codegenState.secTextCode, text);
}

void appendNodeCode(Node* node, char* text) {
  if(strlen(node->cgData->code) + strlen(text) + 10
     > node->cgData->maxCode) {
    node->cgData->code = (char*) realloc(node->cgData->code,
      sizeof(char) * node->cgData->maxCode * 2);
    node->cgData->maxCode *= 2;
  }

  strcat(node->cgData->code, text);
}

void initializeSections() {
  codegenState.maxLenBss = INITIAL_BSSSEC_LEN;
  codegenState.maxLenText = INITIAL_TEXTSEC_LEN;

  codegenState.secBssCode = (char*) malloc(
    sizeof(char) * codegenState.maxLenBss);
  codegenState.secTextCode = (char*) malloc(
    sizeof(char) * codegenState.maxLenText);

  codegenState.secBssCode[0] = '\0';
  codegenState.secTextCode[0] = '\0';

  appendSecBss("section .bss\n");
  appendSecText("section .text\n");
  appendSecText("global _start\n");
  appendSecText("_start:\n");
}

void finalizeSections() {
  appendSecText("mov eax, 60\n"); // exit syscall
  appendSecText("mov edi, 0\n");
  appendSecText("syscall\n");
}

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
      printRegs();
      return i;
    }
  }

  genericError("Code generation failed: out of registers.");
}

void freeReg(char regNum) {
  if(regNum < N_GPR) codegenState.busyRegisters[regNum] = 0;
  else genericError("Code generation bug: freeing invalid register.");

  printRegs();
}

void createCgData(Node* node) {
  if(!node->cgData) {
    node->cgData = (CgData*) malloc(sizeof(CgData));
    node->cgData->maxCode = INITIAL_CODE_SIZE;
    node->cgData->code = (char*) malloc(sizeof(char) * node->cgData->maxCode);
  }
}

char* getSymbolRef(Symbol* sym) {
  if(sym->type == STGlobal) {
    char* ref = (char*) malloc(sizeof(char) * (sym->token->nameSize + 10));
    sprintf(ref, "[rel %s]", sym->token->name);
    return ref;
  }
  return NULL;
}

void printNodeCode(Node* node) {
  if(cli.outputType > OUT_DEFAULT) return; 
  if(cli.outputType <= OUT_DEBUG) printf("> ");

  if(node->cgData) {
    printf("%s", node->cgData->code);
  } 
  else if(cli.outputType <= OUT_DEBUG) printf("\n");
}

void printRegs() {
  if(cli.outputType > OUT_DEBUG) return; 
  for(int i = 0; i < N_GPR; i++) {
    if(codegenState.busyRegisters[i]) printf("1");
    else printf("0");
  }
  printf("\n");
}

void printSections() {
  if(cli.outputType > OUT_DEFAULT) return; 
  printf("%s\n", codegenState.secBssCode);
  printf("%s\n", codegenState.secTextCode);
}

