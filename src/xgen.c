#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "xgen.h"

#define TEMP_DIR ".ulpc-temp"
#define ASM_FILE "generated.asm"
#define OBJ_FILE "obj.o"
#define EXEC_FILE "a.out"
#define ASSEMBLER_CMD "nasm"
#define ASSEMBLER_OPT "-felf64"
#define LINKER_CMD "ld"

char TEMP_DIR_EXISTED;

void writeAsmFile(char* assemblyCode);
void createObjectFile();
void linkObject();
void createTempDir();
void removeTempDir();

void generateExec(char* filename, char* assemblyCode, char* outputName) {
  createTempDir();
  writeAsmFile(assemblyCode);
  createObjectFile();
  linkObject(outputName);

  removeTempDir();
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
  int ret = system(ASSEMBLER_CMD " " ASSEMBLER_OPT " " TEMP_DIR "/" ASM_FILE 
    " -o " TEMP_DIR "/" OBJ_FILE);

  if(ret != 0) { // error: stop
    printf("Assembler error. Object file not created.\n");
    removeTempDir();
    exit(1);
  }
}

void linkObject(char* outputName) {
  int ret;

  if(outputName == NULL) { // no output specified
    ret = system(LINKER_CMD " " TEMP_DIR "/" OBJ_FILE " -o " EXEC_FILE);
  } else { // use specified output name
    char* fmt = LINKER_CMD " " TEMP_DIR "/" OBJ_FILE " -o %s";
    char* cmdStr = (char*) malloc(sizeof(char) * (strlen(LINKER_CMD) + 
      strlen(TEMP_DIR) + strlen(OBJ_FILE) + strlen(outputName) + 10));
    sprintf(cmdStr, fmt, outputName);
    ret = system(cmdStr);
    free(cmdStr);
  }

  if(ret != 0) { // error: stop
    printf("Linker error. Executable file not created.\n");
    removeTempDir();
    exit(1);
  }
}

void createTempDir() {
  TEMP_DIR_EXISTED = 0;
  struct stat s;
  int stat_ret = stat(TEMP_DIR, &s);

  if(stat_ret == -1) {
    if(errno == ENOENT) { // directory does not exist, create it
      mkdir(TEMP_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
    } else {
      printf("Error creating temporary build directory.\n");
      exit(1);
    }
  } else {
    if(S_ISDIR(s.st_mode)) { // directory exists, use it
      TEMP_DIR_EXISTED = 1;
    } else { // exists but it is not a directory
      printf("Error: there exists a file with the same name as the "
        "temporary build directory.\n");
      exit(1);
    }
  }
}

void removeTempDir() {
  if(!TEMP_DIR_EXISTED) system("rm -rf " TEMP_DIR);
}

