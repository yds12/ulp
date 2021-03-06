/*
 *
 *
 * This file centralizes all the main data structures used by the compiler
 * across several phases, such as tokens and the AST.
 *
 */

#ifndef DATAST_H
#define DATAST_H

// A node type name cannot exceed this length
#define MAX_NODE_NAME 40

// Represents a token.
typedef struct stToken {
  char * name;
  int nameSize;
  short type;
  int lnum;
  int chnum;
} Token;

// Represents the types of tokens allowed in the language.
typedef enum enTokenType {
  TTUnknown,
  TTEof,
  TTId,

  // literals
  TTLitInt,
  TTLitString,
  TTLitFloat,
  TTLitBool,
  TTLitArray,

  // structural
  TTLPar,
  TTRPar,
  TTLBrace,
  TTRBrace,
  TTSemi,
  TTColon,
  TTComma,
  TTArrow,

  // operators
  TTDiv,
  TTPlus,
  TTMinus,
  TTMod,
  TTMult,
  TTGreater,
  TTGEq,
  TTLess,
  TTLEq,
  TTEq,
  TTAssign,
  TTIncr,
  TTDecr,
  TTAdd,
  TTSub,

  // keywords
  TTIf,
  TTElse,
  TTFor,
  TTFunc,
  TTWhile,
  TTNext,
  TTBreak,
  TTInt,
  TTString,
  TTBool,
  TTFloat,
  TTAnd,
  TTOr,
  TTNot,
  TTReturn,
  TTLoop,
  TTMatch,
  TTTrue,
  TTFalse
} TokenType;

typedef enum enNodeType {
  NTProgram,
  NTProgramPart,
  NTStatement,
  NTFunction,
  NTDeclaration,
  NTBreakSt,
  NTNextSt,
  NTIfSt,
  NTLoopSt,
  NTWhileSt,
  NTMatchSt,
  NTCallSt,
  NTReturnSt,
  NTForSt,
  NTAssignment,
  NTMatchClause,
  NTExpression,
  NTCallExpr,
  NTCallParam,
  NTArgList,
  NTArg,
  NTNoop,
  NTTerm,
  NTType,
  NTLiteral,
  NTBinaryOp,
  NTIdentifier,
  NTTerminal,
} NodeType;

// Types of symbols
typedef enum enSymbolType {
  STArg,  // function argument
  STLocal,  // local variable (in a function)
  STGlobal,  // global variable (in root scope)
  STFunction  // function name (also in root scope)
} SymbolType;

typedef struct stSymbol {
  Token* token;  // holds the name of the symbol
  short type;  // type of symbol
  short pos;  // if function argument or local variable, the position
} Symbol;

typedef struct stSymbolTable {
  Symbol** symbols;
  int nSymbols;
  int maxSize;
  int nLocalVars;  // number of local variables
  int nArgs;  // number of arguments (for functions)
  int nStackVarsAcc;  // number of stack variables accumulated (+ ancestors)
  int nStackVars;  // number of variables to allocate in stack
} SymbolTable;

typedef struct stCgData {
  char reg;
  char* code;
  int maxCode;
  char* breakLabel;  // label to jump to if break is encountered
  char* nextLabel;  // label to jump to if next is encountered
} CgData;

// Represents a node of the Abstract Syntax Tree (AST)
typedef struct stNode {
  NodeType type;
  Token* token;
  struct stNode** children;
  int nChildren;
  int id;
  struct stNode* parent;
  SymbolTable* symTable;
  CgData* cgData;
} Node;

#endif

