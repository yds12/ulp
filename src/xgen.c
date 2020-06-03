#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "xgen.h"

#define TEMP_DIR ".ulpc-temp"
#define ASM_FILE "generated.asm"
#define OBJ_FILE "obj.o"
#define EXEC_FILE "a.out"
#define ASSEMBLER_CMD "nasm"
#define ASSEMBLER_OPT "-felf64"
#define LINKER_CMD "ld"

void writeAsmFile(char* assemblyCode);
void createObjectFile();
void linkObject();

void generateExec(char* filename, char* assemblyCode) {
  mkdir(TEMP_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
  writeAsmFile(assemblyCode);
  createObjectFile();
  linkObject();
//  system("mkdir .ulpc-temp");
  system("rm -rf " TEMP_DIR);
}

void writeAsmFile(char* assemblyCode) {
  FILE* asmFile = fopen(TEMP_DIR "/" ASM_FILE, "w");
  if(!asmFile) {
    printf("Error creating temporary assembly file.\n");
    exit(1);
  }

  fprintf(asmFile, "%s", assemblyCode);
  fclose(asmFile);
}

void createObjectFile() {
  system(ASSEMBLER_CMD " " ASSEMBLER_OPT " " TEMP_DIR "/" ASM_FILE 
    " -o " TEMP_DIR "/" OBJ_FILE);
}

void linkObject() {
  system(LINKER_CMD " " TEMP_DIR "/" OBJ_FILE " -o " EXEC_FILE);
}

