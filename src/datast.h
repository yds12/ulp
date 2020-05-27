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
  TTNot
} TokenType;

typedef enum stNodeType {
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
  NTCallExpr,
  NTAssignment,
  NTCallParam,
  NTParam,
  NTNoop,
  NTExpression,
  NTTerm,
  NTType,
  NTLiteral,
  NTBinaryOp,
  NTIdentifier,
  NTTerminal,
} NodeType;

// Represents a node of the Abstract Syntax Tree (AST)
typedef struct stNode {
  NodeType type;
  Token* token;
  struct stNode** children;
  int nChildren;
  int id;
} Node;

#endif
