# Compiler Project

This project is for learning purposes only.

We implement a compiler for a simple language. The compiler will be divided
into *lexer* (process the input and generates a list of tokens), *parser*
(generates an 
[Abstract Syntax Tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree)
-- or AST) and *code generator* (generates x64 assembly code targeted at
Linux). The resulting assembly will be processed by `nasm` into object 
code, and then linked with `ld`.
