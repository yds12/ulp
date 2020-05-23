/*
 * Compiler main file.
 */

#include <stdio.h>
#include "lexer.h"

/*
 * The main function should receive the source file (but it can be ommited
 * if a source file is piped into the program).
 *
 */
int main(int argc, char ** argv) {
  FILE* sourcefile;
  char* filename;

  if(argc < 2) { // read from stdin
    sourcefile = stdin;
    filename = "STDIN";
  } else { // read specified file
    sourcefile = fopen(argv[1], "rt");
    filename = argv[1];
  }

  lexerStart(sourcefile, filename);
  return 0;
}
