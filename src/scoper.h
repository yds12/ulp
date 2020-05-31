/*
 *
 *
 * Scope checker.
 *
 */

#ifndef SCOPER_H
#define SCOPER_H

#include <stdio.h>
#include "datast.h"

typedef struct stScoperState {
  FILE* file;
  char* filename;
} ScoperState;

ScoperState scoperState;

void scopeCheckerStart(FILE* file, char* filename, Node* ast);

#endif

