
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
void appendNodeCode(Node* node, char* text);
void declareGlobalVar(Node* node, char* varName, char size);
char* getSymbolRef(Symbol* sym);
char* getSymbolSizeRef(Symbol* sym);
void printRegs();
void printNodeCode(Node* node);
void pullChildCode(Node* node, int childNumber);

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
        // TODO for now this only works for int
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

          appendNodeCode(node, node->children[0]->cgData->code);
          appendNodeCode(node, node->children[2]->cgData->code);
          free(node->children[0]->cgData->code);
          free(node->children[2]->cgData->code);

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
      pullChildCode(node, 2);
      appendInstruction(node, INS_MOV,
        getSymbolRef(varSym),
        getRegName(node->children[2]->cgData->reg));
      freeNodeReg(node->children[2]);
    }
  }
  else if(node->type == NTAssignment) {
    if(node->nChildren == 3) {
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
      pullChildCode(node, 2);

      if(node->children[1]->token->type == TTAssign) {
        appendInstruction(node, INS_MOV,
          getSymbolRef(varSym),
          getRegName(node->children[2]->cgData->reg));
      }
      else if(node->children[1]->token->type == TTAdd ||
              node->children[1]->token->type == TTSub) {
        InstructionType iType = 
          (node->children[1]->token->type == TTAdd) ? INS_ADD : INS_SUB;

        allocateReg(node);
        appendInstruction(node, INS_MOV,
          getRegName(node->cgData->reg),
          getSymbolRef(varSym));
        appendInstruction(node, iType,
          getRegName(node->cgData->reg),
          getRegName(node->children[2]->cgData->reg));
        appendInstruction(node, INS_MOV,
          getSymbolRef(varSym),
          getRegName(node->cgData->reg));
        freeNodeReg(node);
      }

      freeNodeReg(node->children[2]);
    }
    else if(node->nChildren == 2) { // x++  or  x--
      if(node->children[1]->type != NTTerminal)
        genericError("Code generation bug: terminal symbol expected.");

      InstructionType iType = INS_INC;
      if(node->children[1]->token->type == TTDecr) iType = INS_DEC;

      if(node->children[0]->nChildren < 1)
        genericError("Code generation bug: AST identifier node without child.");

      if(!node->children[0]->children[0]->token)
        genericError("Code generation bug: AST node missing token.");

      Token* varToken = node->children[0]->children[0]->token;
      Symbol* varSym = lookupSymbol(node, varToken);

      createCgData(node);
      appendInstruction(node, iType, getSymbolSizeRef(varSym), NULL);
    }
  }
  else if(node->type == NTStatement) {
    createCgData(node);

    if(node->nChildren == 1) { // pulls code from child
      pullChildCode(node, 0);
    }
  }
  else if(node->type == NTNoop) {
    createCgData(node);
    appendInstruction(node, INS_NOP, NULL, NULL);
  }
  else if(node->type == NTProgramPart) {
    createCgData(node);
    pullChildCode(node, 0);
  }
  else if(node->type == NTProgram) {
    createCgData(node);

    // .bss section
    if(node->symTable) {
      appendNodeCode(node, "section .bss\n");

      for(int i = 0; i < node->symTable->nSymbols; i++) {
        if(node->symTable->symbols[i]->type == STGlobal) {
          declareGlobalVar(node, node->symTable->symbols[i]->token->name, 4);
        }
      }
    }

    // .text section header
    appendNodeCode(node, "section .text\n");
    appendNodeCode(node, "global _start\n");
    appendNodeCode(node, "_start:\n");

    // pulls code from children
    for(int i = 0; i < node->nChildren; i++) {
      if(node->children[i]->cgData && node->children[i]->cgData->code) {
        pullChildCode(node, i);
      }
    }

    // exit syscall
    appendNodeCode(node, "mov eax, 60\n");
    appendNodeCode(node, "mov edi, 0\n");
    appendNodeCode(node, "syscall\n");

    if(cli.outputType < OUT_SILENT) printNodeCode(node);
  }
}

void pullChildCode(Node* node, int childNumber) {
  if(node->children[childNumber]->cgData && 
     node->children[childNumber]->cgData->code) {
    appendNodeCode(node, node->children[childNumber]->cgData->code);
    free(node->children[childNumber]->cgData->code);
  }
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
  if(!node->cgData) createCgData(node);

  char instructionStr[MAX_INSTRUCTION_LEN];
  char* fmt;

  switch(inst) {
    case INS_MOV: 
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand.");
      fmt = "mov %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2); 
      break;
    case INS_ADD: 
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand.");
      fmt = "add %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2); 
      break;
    case INS_SUB: 
      if(!op1 || !op2)
        genericError("Code generation bug: empty instruction operand.");
      fmt = "sub %s, %s\n";
      sprintf(instructionStr, fmt, op1, op2); 
      break;
    case INS_NOP:
      strcpy(instructionStr, "nop\n");
      break;
    case INS_INC:
      if(!op1) genericError("Code generation bug: empty instruction operand.");
      fmt = "inc %s\n";
      sprintf(instructionStr, fmt, op1); 
      break;
    case INS_DEC:
      if(!op1) genericError("Code generation bug: empty instruction operand.");
      fmt = "dec %s\n";
      sprintf(instructionStr, fmt, op1); 
      break;
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
    node->cgData->code[0] = '\0';
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

char* getSymbolSizeRef(Symbol* sym) {
  // TODO for now the size is fixed. Should be decided according to data type.
  if(sym->type == STGlobal) {
    char* ref = (char*) malloc(sizeof(char) * (sym->token->nameSize + 20));
    sprintf(ref, "dword [rel %s]", sym->token->name);
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

