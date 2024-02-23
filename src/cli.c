#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cli.h"

struct stCli cli;

void processCLArg(char* arg, int index);
void setDefaultOptions();
void displayHelp();
void displayVersion();

void parseCLArgs(int argc, char ** argv) {
  setDefaultOptions();
  if(argc < 2) { // nothing to do
    cli.sourceIdx = -1;
    return;
  }
  for(int i = 1; i < argc; i++) processCLArg(argv[i], i);
}

void setDefaultOptions() {
  cli = (struct stCli) {
    .outputType = OUT_DEFAULT,
    .sourceIdx = -1,
    .outputIdx = -1
  };
}

void processCLArg(char* arg, int index) {
  int len = strlen(arg);
  if(len <= 0) return;
  if(index == cli.outputIdx) return;

  if(arg[0] == '-') { // command line option
    switch(len) {
      case 2:
        if(arg[1] == 's') cli.outputType = OUT_SILENT;
        else if(arg[1] == 'h') displayHelp();
        else if(arg[1] == 'V') displayVersion();
        else if(arg[1] == 'v') cli.outputType = OUT_VERBOSE;
        else if(arg[1] == 'o') cli.outputIdx = index + 1;
        break;
      case 6:
        if(strncmp("--help", arg, len) == 0)
        displayHelp();
        break;
      case 8:
        if(strncmp("--silent", arg, len) == 0)
          cli.outputType = OUT_SILENT;
        else if(strncmp("--cdebug", arg, len) == 0)
          cli.outputType = OUT_DEBUG;
        break;
      case 9:
        if(strncmp("--version", arg, len) == 0) {
          displayVersion();
        } else if(strncmp("--verbose", arg, len) == 0) {
          cli.outputType = OUT_VERBOSE;
        }
        break;
      case 10:
        if(strncmp("--graphviz", arg, len) == 0)
          cli.outputType = OUT_GRAPHVIZ;
        break;
    }

    return;
  }
  cli.sourceIdx = index; // this arg is the filename
}

void displayHelp() {
  printf("ulpc -- The ulp compiler.\n"
    "Version: " VERSION "\n"
    "Usage: ulpc [options] file\n"
    "Options:\n"
    "  --cdebug\t\tDebug mode. Displays lots of compiler debug information.\n"
    "  --graphviz\t\tOnly parses and outputs the AST in graphviz format.\n"
    "  --help, -h\t\tDisplays this help message.\n"
    "  -o <file>\t\tSets <file> as the output file.\n"
    "  --silent, -s\t\tNo output (to stdout).\n"
    "  --verbose, -v\t\tDetailed output.\n"
    "  --version, -V\t\tDisplays the compiler version.\n"
  );
  exit(0);
}

void displayVersion() {
  printf(VERSION "\n");
  exit(0);
}

