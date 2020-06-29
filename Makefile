ODir=build
BDir=build
SDir=src
Exec=build/ulpc
CC=gcc
Sources=src/*.c
Opt=-g
RelOpt=-O3

compile:
	$(CC) $(Opt) $(Sources) -o $(Exec) 

release:
	$(CC) $(RelOpt) $(Sources) -o $(Exec) 

clean:
	-rm -f $(BDir)/* $(ODir)/* $(Exec)

rebuild: clean compile
