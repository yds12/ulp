CC=gcc
CFlags=-std=c99 -g -O3 -Wall -Wextra
BDir=build
SDir=src
Exec=$(BDir)/ulpc
Sources=$(wildcard $(SDir)/*.c)
Objects=$(patsubst $(SDir)/%.c, $(BDir)/%.o,$(Sources))

all: $(Exec)

$(Exec): $(Objects)
	$(CC) -o $(Exec) $(Objects)

$(BDir)/%.o:$(SDir)/%.c
	$(CC) -c $(CFlags) -o $@ $^

test: $(Exec)
	@./aux/test

clean:
	rm -f $(BDir)/* $(Exec) a.out

rebuild: clean $(Exec)

.PHONY: clean rebuild

