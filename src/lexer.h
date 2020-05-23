void lexerStart(FILE* sourcefile, char* filename);

char* filename;

typedef struct stToken {
  char * name;
  int nameSize;
  short type;
  int lnum;
  int chnum;
} Token;

typedef struct stLexerState {
  FILE* file;
  char lastChar;
  int lnum;      // this should always point to the line of the last char read
  int chnum;     // this should always point to the position of the last char
  int prevLnum;
  int prevChnum;
} LexerState;

LexerState lexerState;

Token* tokens;
int n_tokens;

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

void printFile(FILE* file);
void printTokenInFile(FILE* file, Token token);
void error(char* msg);
int isWhitespace(char character);
int isAlpha(char character);
int isNum(char character);
int isSingleCharOp(char character);
int belongsToDoubleOp(char character);
