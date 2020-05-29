/*
 * Compiler main file.
 */

#include <stdio.h>
#include "cli.h"
#include "lexer.h"
#include "parser.h"

/*
 * The main function should receive the source file (but it can be ommited
 * if a source file is piped into the program).
 *
 */
int main(int argc, char ** argv) {
  int filenameIdx = parseCLArgs(argc, argv);

  FILE* sourcefile;
  char* filename;

  if(filenameIdx < 1) { // read from stdin
    sourcefile = stdin;
    filename = "<stdin>";
  } else { // read specified file
    sourcefile = fopen(argv[filenameIdx], "rt");
    filename = argv[filenameIdx];
  }

  lexerStart(sourcefile, filename);
  parserStart(sourcefile, filename, lexerState.nTokens, lexerState.tokens);
  fclose(sourcefile);
  return 0;
}
