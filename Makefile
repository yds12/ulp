ODir=bin
BDir=bin
SDir=src
Exec=bin/compiler
CC=gcc
Sources=src/*.c
Opt=-g

build:
	$(CC) $(Opt) $(Sources) -o $(Exec) 

clean:
	-rm -f $(BDir)/* $(ODir)/* $(Exec)

rebuild: clean build
