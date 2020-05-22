void tokenizer_start(FILE* sourcefile, char* filename);

typedef struct stToken {
  char * name;
  int name_size;
  short type;
  int lnum;
  int chnum;
} Token;

typedef enum enTokenType {
  TTUnknown,
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

  // operators
  TTDiv,
  TTPlus,
  TTMinus,
  TTMod,
  TTMult,
  TTAssign,
  TTEq,
  TTIncr,
  TTDecr,

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

Token* tokens;
int n_tokens;
