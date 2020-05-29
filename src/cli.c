#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cli.h"

int processCLArg(char* arg);
void setDefaultOptions();
void displayHelp();
void displayVersion();

int parseCLArgs(int argc, char ** argv) {
  setDefaultOptions();
  int fileIdx = -1;
  if(argc < 2) return fileIdx; // nothing to do

  for(int i = 1; i < argc; i++) { // process each arg.
    if(processCLArg(argv[i])) fileIdx = i;
  }
  return fileIdx;
}

void setDefaultOptions() {
  cli = (struct stCli) {
    .outputType = OUT_DEFAULT
  };
}

int processCLArg(char* arg) {
  int len = strlen(arg);
  if(len <= 0) return 0;

  if(arg[0] == '-') { // command line option
    switch(len) {
      case 2:
        if(arg[1] == 's') cli.outputType = OUT_SILENT;
        else if(arg[1] == 'h') displayHelp();
        else if(arg[1] == 'V') displayVersion();
        else if(arg[1] == 'v') cli.outputType = OUT_VERBOSE;
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

    return 0;
  }
  return 1; // this arg is the filename
}

void displayHelp() {
  printf("ulpc -- The ulp compiler.\nVersion: " VERSION "\n");
  exit(0);
}

void displayVersion() {
  printf(VERSION "\n");
  exit(0);
}

