#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 100000
#define TOKENS_SIZE 1000

#define TTYPE_UNKNOWN -1
#define TTYPE_ID 0
#define TTYPE_INT 1
#define TTYPE_FLOAT 2
#define TTYPE_STRING 3

typedef struct stToken {
  char * name;
  int name_size;
  short type;
} Token;

Token addToken(char * buffer, int size);

void tokenizer_start(FILE* sourcefile) {
  // The list of tokens in the source file
  Token tokens[TOKENS_SIZE];
  int n_tokens = 0;

  // Buffer to read the chars, is reset at the end of each token
  char char_buffer[BUFFER_SIZE];
  int i = 0;

  char ch;
  while(!feof(sourcefile)) {
    ch = fgetc(sourcefile);
    printf("%c", ch);
    char_buffer[i] = ch;

    if(ch == ' ' || ch == '\n') {
      if(i > 0) { // end of a token
        Token token = addToken(char_buffer, i);
        tokens[n_tokens] = token;
        n_tokens++;
      }
      i = 0; // restart buffer
      continue;
    }

    if(i == BUFFER_SIZE) {
      printf("File too long. Buffer overflowed.");
    }

    i++;
  }

  printf("Total tokens: %d\n", n_tokens);

  for(int i = 0; i < n_tokens; i++) {
    printf("Type: %d\tSize: %d\t\tName: %s\n", tokens[i].type, 
      tokens[i].name_size, tokens[i].name);
  }
}

Token addToken(char * buffer, int size) {
//  printf("Generating token:%.*s\n", size, buffer);

  char* tokenName = (char*) malloc((size + 1) * sizeof(char));
//  char tokenName[size + 1];
  Token token = { tokenName, size, TTYPE_UNKNOWN };
  strncpy(tokenName, buffer, size);
  token.name[size] = '\0';
  return token;
}
