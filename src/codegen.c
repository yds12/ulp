
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

char getReg();
char* getRegName(char regNum);
void freeReg(char regNum);
void freeNodeReg(Node* node);
void emitCode(Node* node);
void emitExprCode(Node* node);
void emitAssignCode(Node* node);
void emitIfCode(Node* node);
void emitFunctionCode(Node* node);
void emitCallCode(Node* node);
void emitDeclarationCode(Node* node);
void emitStatementCode(Node* node);
void emitProgramCode(Node* node);
void createCgData(Node* node);
void allocateReg(Node* node);
void printRegs();
void printNodeCode(Node* node);
void pullChildCode(Node* node, int childNumber);
char* getLabel();

void codegenStart(FILE* file, char* filename, Node* ast) {
  codegenState = (CodegenState) {
    .file = file,
    .filename = filename,
    .code = NULL,
    .nLabels = 0
  };

  initializeRegisters();
  postorderTraverse(ast, &emitCode);

  if(ast->cgData && ast->cgData->code) {
    codegenState.code = ast->cgData->code;
    printNodeCode(ast);
  } 
  else genericError("Code generator bug: no code generated");
}

void emitCode(Node* node) {
  if(node->type == NTExpression) {
    emitExprCode(node);
  }
  else if(node->type == NTCallParam) { // argument in function call
    createCgData(node);
    node->cgData->reg = node->children[0]->cgData->reg;
    pullChildCode(node, 0);
  }
  else if(node->type == NTCallSt || node->type == NTCallExpr) {
    emitCallCode(node);
  }
  else if(node->type == NTDeclaration) {
    emitDeclarationCode(node);
  }
  else if(node->type == NTAssignment) {
    emitAssignCode(node);
  }
  else if(node->type == NTIfSt) {
    emitIfCode(node);
  }
  else if(node->type == NTReturnSt) {
    createCgData(node);

    if(node->nChildren > 0) {
      pullChildCode(node, 0);
      appendInstruction(node, INS_SETRET, 
        getRegName(node->children[0]->cgData->reg), NULL);
      freeNodeReg(node->children[0]);
    }

    appendInstruction(node, INS_JMP, ".epilogue", NULL);
  }
  else if(node->type == NTStatement) {
    emitStatementCode(node);
  }
  else if(node->type == NTNoop) {
    createCgData(node);
    appendInstruction(node, INS_NOP, NULL, NULL);
  }
  else if(node->type == NTFunction) {
    emitFunctionCode(node);
  }
  else if(node->type == NTProgramPart) {
    createCgData(node);
    pullChildCode(node, 0);
  }
  else if(node->type == NTProgram) {
    emitProgramCode(node);
  }
}

void emitCallCode(Node* node) {
  createCgData(node);

  if(node->nChildren < 1)
    genericError("Code generation bug: AST call node without function name.");

  if(!node->children[0]->children[0]->token)
    genericError("Code generation bug: AST node missing token.");

  // pulls code for each argument expression
  if(node->nChildren > 1) {
    for(int i = 0; i < node->nChildren - 1; i++) {
      pullChildCode(node, i + 1);
    }
  }

  // copy arguments to the correct registers
  if(node->nChildren > 1) {
    for(int i = 0; i < node->nChildren - 1; i++) {
      appendInstruction(node, INS_MOV, 
        getArgRegName(i),
        getRegName(node->children[i + 1]->cgData->reg));
      freeNodeReg(node->children[i + 1]);
    }
  }

  // call instruction
  char* funcName = node->children[0]->children[0]->token->name;
  appendInstruction(node, INS_CALL, funcName, NULL);

  // copy return value to a register
  if(node->type == NTCallExpr) {
    allocateReg(node);
    appendInstruction(node, INS_GETRET, getRegName(node->cgData->reg), NULL);
  }
}

void emitDeclarationCode(Node* node) {
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
      getSymbolRef(varSym, node),
      getRegName(node->children[2]->cgData->reg));
    freeNodeReg(node->children[2]);
  }
}

void emitStatementCode(Node* node) {
  createCgData(node);
  if(node->nChildren == 1) { 
    pullChildCode(node, 0);
  } else {
    char statementBlock = 1;

    for(int i = 0; i < node->nChildren; i++) {
      if(node->children[i]->type != NTStatement) statementBlock = 0;
    }

    char hasEpilogue = 0;

    if(statementBlock) {
      if(isMlsNode(node) && node->parent->type != NTFunction 
         && node->symTable) { 
        hasEpilogue = 1;
        // Save stack pointer as base pointer
        appendInstruction(node, INS_PUSH, "rbp", NULL);
        appendInstruction(node, INS_MOV, "rbp", "rsp");

        // allocate space in the stack for the variables
        // TODO: size 4 fixed here
        // TODO: remove mention to RSP from here
        char stackSpace[10];
        sprintf(stackSpace, "%d", node->symTable->nLocalVars * 4);
        appendInstruction(node, INS_SUB, "rsp", stackSpace);
      }

      // pulls code from children statements
      for(int i = 0; i < node->nChildren; i++) {
        pullChildCode(node, i);
      }

      if(hasEpilogue) {
        appendInstruction(node, INS_POP, "rbp", NULL);
      }
    }
  }
}

void emitProgramCode(Node* node) {
  createCgData(node);

  // .bss section
  if(node->symTable) {
    appendInstruction(node, INS_SECTION, "bss", NULL);

    for(int i = 0; i < node->symTable->nSymbols; i++) {
      if(node->symTable->symbols[i]->type == STGlobal) {
        declareGlobalVar(node, node->symTable->symbols[i]->token->name, 4);
      }
    }
  }

  // .text section header
  appendInstruction(node, INS_SECTION, "text", NULL);
  appendInstruction(node, INS_GLOBAL, "_start", NULL);

  // first pulls code from functions
  for(int i = 0; i < node->nChildren; i++) {
    if(node->children[i]->type == NTProgramPart
       && node->children[i]->nChildren == 1
       && node->children[i]->children[0]->type == NTFunction
       && node->children[i]->cgData 
       && node->children[i]->cgData->code) {
      pullChildCode(node, i);
    }
  }

  appendInstruction(node, INS_LABEL, "_start", NULL);

  // then pulls code from other children
  for(int i = 0; i < node->nChildren; i++) {
    if(node->children[i]->type == NTProgramPart
       && node->children[i]->nChildren == 1
       && node->children[i]->children[0]->type != NTFunction
       && node->children[i]->cgData 
       && node->children[i]->cgData->code) {
      pullChildCode(node, i);
    }
  }

  // exit syscall
  appendInstruction(node, INS_EXIT, "0", NULL);
}

void emitFunctionCode(Node* node) {
  createCgData(node);

  // create label with function name
  if(node->nChildren != 3)
    genericError("Code generator bug: bad function AST node (missing "
      "children).");

  if(node->children[0]->type != NTIdentifier)
    genericError("Code generator bug: bad function AST node (identifier "
      "expected).");

  if(node->children[1]->type != NTArgList)
    genericError("Code generator bug: bad function AST node (parameter "
      "declaration expected).");

  if(node->children[2]->type != NTStatement)
    genericError("Code generator bug: bad function AST node (statement "
      "expected).");

  if(node->children[0]->nChildren != 1)
    genericError("Code generator bug: bad function AST node (ID node "
      "without child node).");

  if(!node->children[0]->children[0]->token)
    genericError("Code generator bug: bad function AST node (terminal node "
      "without token).");

  char* fName = node->children[0]->children[0]->token->name;
  appendInstruction(node, INS_LABEL, fName, NULL);
  
  // allocate stack space for arguments and local variables if needed
  char stackSpace[10];
  Node* mlsNode = getMlsNode(node->children[2]);

  if(mlsNode && mlsNode->symTable) {
    sprintf(stackSpace, "%d", mlsNode->symTable->nStackVars * 4);
    appendInstruction(node, INS_PROLOGUE_STACK, stackSpace, NULL);

    // move arguments to stack
    for(int i = 0; i < mlsNode->symTable->nSymbols; i++) {
      Symbol* argSym = mlsNode->symTable->symbols[i];
      if(argSym->type == STArg) {
        char* regName = getArgRegName(argSym->pos);
        appendInstruction(node, INS_MOV, 
          getSymbolRef(argSym, node),
          regName);
        free(regName);
      }
    }
  }
  else appendInstruction(node, INS_PROLOGUE, NULL, NULL);

  // pull code from function body
  pullChildCode(node, 2);
  appendInstruction(node, INS_EPILOGUE, NULL, NULL);
}

void emitIfCode(Node* node) {
  // TODO: so far only works with binary EXPR as conditional
  if(node->nChildren < 2)
    genericError("Compiler bug: AST 'if' node missing children.");

  Node* condNode = node->children[0];
  Node* thenNode = node->children[1];
  Node* elsedNode = node->children[2];

  createCgData(node);
  pullChildCode(node, 0); // comparison

  if(condNode->type != NTExpression)
    genericError("Compiler bug: expression not found for 'if' condition.");

  if(condNode->nChildren == 3) { // binary expression
    if(condNode->children[1]->nChildren < 1 
       || condNode->children[1]->children[0]->type != NTTerminal
       || !condNode->children[1]->children[0]->token)
      genericError("Compiler bug: operator missing.");

    char hasElse = (node->nChildren == 3);
    char* elseLabel = getLabel();
    char* endLabel = getLabel();

    TokenType opType = condNode->children[1]->children[0]->token->type;
    char* jmpTo = endLabel;
    if(hasElse) jmpTo = elseLabel;

    switch(opType) {
      case TTEq: appendInstruction(node, INS_JNE, jmpTo, NULL); break;
      case TTGreater: appendInstruction(node, INS_JLE, jmpTo, NULL); break;
      case TTGEq: appendInstruction(node, INS_JL, jmpTo, NULL); break;
      case TTLess: appendInstruction(node, INS_JGE, jmpTo, NULL); break;
      case TTLEq: appendInstruction(node, INS_JG, jmpTo, NULL); break;
    }

    pullChildCode(node, 1); // THEN code

    if(hasElse) {
      appendInstruction(node, INS_JMP, endLabel, NULL);
      appendInstruction(node, INS_LABEL, elseLabel, NULL);
      pullChildCode(node, 2); // ELSE code
    }

    appendInstruction(node, INS_LABEL, endLabel, NULL);

    free(elseLabel);
    free(endLabel);
  }
}

void emitAssignCode(Node* node) {
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
        getSymbolRef(varSym, node),
        getRegName(node->children[2]->cgData->reg));
    }
    else if(node->children[1]->token->type == TTAdd ||
            node->children[1]->token->type == TTSub) {
      InstructionType iType = 
        (node->children[1]->token->type == TTAdd) ? INS_ADD : INS_SUB;

      allocateReg(node);
      appendInstruction(node, INS_MOV,
        getRegName(node->cgData->reg),
        getSymbolRef(varSym, node));
      appendInstruction(node, iType,
        getRegName(node->cgData->reg),
        getRegName(node->children[2]->cgData->reg));
      appendInstruction(node, INS_MOV,
        getSymbolRef(varSym, node),
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
    appendInstruction(node, iType, getSymbolSizeRef(varSym, node), NULL);
  }
}

void emitExprCode(Node* node) {
  if(node->nChildren == 1) {
    Node* litOrIdNode = node->children[0];
    Token* token = litOrIdNode->children[0]->token;

    if(node->children[0]->type == NTLiteral) {
      // TODO for now this only works for int and bool
      createCgData(node);
      allocateReg(node);

      char* litValue = token->name;
      if(token->type == TTTrue) litValue = "1";
      else if(token->type == TTFalse) litValue = "0";

      appendInstruction(node, INS_MOV, 
        getRegName(node->cgData->reg), litValue);
    } else if(node->children[0]->type == NTIdentifier) {
      createCgData(node);
      Symbol* varSym = lookupSymbol(node, token);

      // load the variable in a register
      allocateReg(node);
      appendInstruction(node, INS_MOV,
        getRegName(node->cgData->reg),
        getSymbolRef(varSym, node));
    } else if(node->children[0]->type == NTCallExpr) {
      createCgData(node);
      pullChildCode(node, 0);
      node->cgData->reg = node->children[0]->cgData->reg;
    }
  } 
  else if(node->nChildren == 2) {
    if(node->children[0]->type == NTBinaryOp) {
      Token* opToken = node->children[0]->children[0]->token;

      if(!opToken)
        genericError("Code generation bug: missing minus token.");

      if(opToken->type == TTMinus) { // - EXPR
        createCgData(node);

        if(!node->children[1]->cgData)
          genericError("Code generation bug: AST node without code info.");

        pullChildCode(node, 1);
        node->cgData->reg = node->children[1]->cgData->reg;
        appendInstruction(node, INS_NEG,
          getRegName(node->children[1]->cgData->reg), NULL);
      }
    } else if(node->children[0]->type == NTTerminal &&
              node->children[0]->token->type == TTNot) { // not EXPR
      createCgData(node);

      if(!node->children[1]->cgData)
        genericError("Code generation bug: AST node without code info.");

      pullChildCode(node, 1);
      node->cgData->reg = node->children[1]->cgData->reg;
      appendInstruction(node, INS_NOT,
        getRegName(node->children[1]->cgData->reg), NULL);
      freeNodeReg(node->children[1]);
    }
  }
  else if(node->nChildren == 3) {
    if(node->children[1]->type == NTBinaryOp) { // binary operation
      Token* opToken = node->children[1]->children[0]->token;
      createCgData(node);

      if(!node->children[0]->cgData || !node->children[2]->cgData) {
        genericError("Code generation bug: AST node without code info.");
      }

      pullChildCode(node, 0);
      pullChildCode(node, 2);

      if(opToken->type == TTDiv || opToken->type == TTMod) { // division/mod
        appendInstruction(node, INS_DIVISION, 
          getRegName(node->children[0]->cgData->reg),
          getRegName(node->children[2]->cgData->reg));

        node->cgData->reg = node->children[0]->cgData->reg;

        InstructionType iType = (opToken->type == TTDiv) ? 
          INS_GETQUOTIENT : INS_GETREMAINDER;

        appendInstruction(node, iType, getRegName(node->cgData->reg), NULL);
        freeNodeReg(node->children[2]);
      } else { // other binary operations
        InstructionType iType;
        
        switch(opToken->type) {
          case TTPlus: iType = INS_ADD; break;
          case TTMinus: iType = INS_SUB; break;
          case TTAnd: iType = INS_AND; break;
          case TTOr: iType = INS_OR; break;
          case TTMult: iType = INS_IMUL; break;
          case TTEq:
          case TTGreater:
          case TTGEq:
          case TTLess:
          case TTLEq:
            iType = INS_CMP;
            break;
          default:
            genericError("Code generation error: invalid binary operation.");
            break;
        }

        node->cgData->reg = node->children[0]->cgData->reg;
        appendInstruction(node, iType,
          getRegName(node->children[0]->cgData->reg),
          getRegName(node->children[2]->cgData->reg));
        freeNodeReg(node->children[2]);
      }
    }
  } 
}

char* getLabel() {
  char* label = (char*) malloc(sizeof(char) * 8);
  sprintf(label, ".l%d", codegenState.nLabels);
  codegenState.nLabels++;
  return label;
}

void pullChildCode(Node* node, int childNumber) {
  if(node->children[childNumber]->cgData && 
     node->children[childNumber]->cgData->code) {
    appendNodeCode(node, node->children[childNumber]->cgData->code);
    free(node->children[childNumber]->cgData->code);
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

void printNodeCode(Node* node) {
  if(cli.outputType > OUT_DEBUG) return; 

  if(node->cgData) {
    printf("\n%s\n", node->cgData->code);
  } 
}

void printRegs() {
  if(cli.outputType > OUT_DEBUG) return; 
  for(int i = 0; i < N_GPR; i++) {
    if(codegenState.busyRegisters[i]) printf("1");
    else printf("0");
  }
  printf("\n");
}

