#include <stdio.h>
#include "tokenizer.h"

int main(int argc, char ** argv) {
  // check arguments
  /*for(int i = 0; i < argc; i++) {
    printf("ARG %d: %s\n\n", i, argv[i]);
  }*/

  FILE* sourcefile;
  char* filename;

  if(argc < 2) { // read from stdin
    //printf("File to process: STDIN\n\n");
    sourcefile = stdin;
    filename = "STDIN";
  } else { // read specified file
    //printf("File to process: %s\n\n", argv[1]);
    sourcefile = fopen(argv[1], "rt");
    filename = argv[1];
  }

  tokenizer_start(sourcefile, filename);
  fclose(sourcefile);
  return 0;
}
