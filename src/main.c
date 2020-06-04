/*
 * Compiler main file.
 */

#include <stdio.h>
#include "cli.h"
#include "lexer.h"
#include "parser.h"
#include "scoper.h"
#include "codegen.h"
#include "xgen.h"

/*
 * The main function should receive the source file (but it can be ommited
 * if a source file is piped into the program).
 *
 */
int main(int argc, char ** argv) {
  parseCLArgs(argc, argv);
  int filenameIdx = cli.sourceIdx;
  int outputIdx = cli.outputIdx;

  FILE* sourcefile;
  char* filename;

  if(filenameIdx < 1) { // read from stdin
    sourcefile = stdin;
    filename = "<stdin>";
  } else { // read specified file
    sourcefile = fopen(argv[filenameIdx], "rt");

    if(!sourcefile) {
      if(cli.outputType <= OUT_DEFAULT)
        printf("ERROR: Invalid file name.\n");

      return 1;
    }

    filename = argv[filenameIdx];
  }

  lexerStart(sourcefile, filename);
  parserStart(sourcefile, filename, lexerState.nTokens, lexerState.tokens);

  // Just generate the parser output for Graphviz
  if(cli.outputType == OUT_GRAPHVIZ) return 0;

  scopeCheckerStart(sourcefile, filename, parserState.ast);
  codegenStart(sourcefile, filename, parserState.ast);

  char* outputName = NULL;
  if(outputIdx >= 0) outputName = argv[outputIdx];
  generateExec(filename, codegenState.code, outputName);

  fclose(sourcefile);
  return 0;
}

