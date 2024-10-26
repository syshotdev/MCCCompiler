#include "c-vector/vec.h"
#include "lexer.h"
#include "parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Here's all the things we need to accomplish:
 * Program-wise:
 * Don't use a whole lot of ram at once
 * Don't use a lot of storage
 * Try to reuse variables
 * Optimize bigger operations
 * Be able to tokenize/lex/use standard C libraries
 */

// I think no memory leaks or segmentation faults. Good luck!
int main(void) {
  char file_name[] = "test/assignment.c";

  FILE *file;
  file = fopen(file_name, "r");

  if (!file) {
    printf("Couldn't find file: %s", file_name);
  }
  token *tokens = lexer(file);
  parser(tokens);

  fclose(file);

  return 0;
}

/*
 * Steps to convert C program to machine code:
 *
 * Lexer - Turns all words and characters into tokens, such as VARIABLE or
 * PUNCTUATOR Preprocesser - Removes comments, replaces macros with actual code
 *
 * AST Generator - Goes through all the tokens and creates a tree-like structure
 * full of nodes Syntax Analyzer - Checks (Via tree) if stuff like quotes are
 * ever closed, and if keywords are used correctly Semantics Analyzer - Is this
 * variable ever assigned? Valid dereference here? Valid pointer to int cast?
 *
 * Code Generator - Turn the Abstract Syntax Tree into assembly (I actually have
 * no idea what happens here) Code Optimizer - Make code faster via speedup
 * tricks??? (Neither this one)
 *
 *
 * Linker - Put all of the libraries codes into executable (Preprocessor but for
 * libraries???) Assembly to Machine Code - Convert all of the assembly
 * operations into 1s and 0s Turn into minecraft barrels - Create schematic of
 * barrels to put into ROM
 *
 * Execute - Run the program on the Minecraft computer and wow it's working!!!
 * Debug - Spoiler alert: It did not work
 *
 * And repeat the cycle!
 */
