void tokenizer_start(FILE* sourcefile, char* filename);

typedef struct stToken {
  char * name;
  int name_size;
  short type;
  int lnum;
  int chnum;
} Token;

Token* tokens;
int n_tokens;
