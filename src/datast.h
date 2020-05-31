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
  NTParams,
  NTParam,
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
} Symbol;

typedef struct stSymbolTable {
  Symbol** symbols;
  int nSymbols;
  int maxSize;
} SymbolTable;

// Represents a node of the Abstract Syntax Tree (AST)
typedef struct stNode {
  NodeType type;
  Token* token;
  struct stNode** children;
  int nChildren;
  int id;
  struct stNode* parent;
  SymbolTable* symTable;
} Node;

#endif

