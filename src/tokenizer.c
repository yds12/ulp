#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 100000
#define TOKENS_SIZE 1000

#define TTYPE_UNKNOWN -1
#define TTYPE_ID 0
#define TTYPE_INT 1
#define TTYPE_FLOAT 2
#define TTYPE_STRING 3

struct Token {
  char * name;
  int nameSize;
  short type;
};

struct Token addToken(char * buffer, int size);

void tokenizer_start(FILE* sourcefile) {
  struct Token tokens[TOKENS_SIZE];
  int n_tokens = 0;

  char char_buffer[BUFFER_SIZE];
  int i = 0;

  char ch = fgetc(sourcefile);
  while(ch != EOF) {
    printf("%c", ch);
    char_buffer[i] = ch;
    i++;
    ch = fgetc(sourcefile);

    if(ch == ' ' || ch == '\n') {
      if(i > 0) { // end of a token
        struct Token token = addToken(char_buffer, i);
        tokens[n_tokens] = token;
        n_tokens++;
      } else { // just an extra space
        i = 0;
      }
    }

    if(i == BUFFER_SIZE) {
      printf("File too long. Buffer overflowed.");
    }
  }

  printf("Total tokens: %d\n", n_tokens);

  for(int i = 0; i < n_tokens; i++) {
    printf("Name: %s Type: %d\n", tokens[i].name, tokens[i].type);
  }
}

struct Token addToken(char * buffer, int size) {
  char tokenName[size + 1];
  struct Token token = { tokenName, size, TTYPE_UNKNOWN };
  strncpy(tokenName, buffer, size);
  token.name[size] = '\0';
  return token;
}
