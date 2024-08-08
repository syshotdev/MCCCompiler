#include "c-vector/vec.h"
#include <ctype.h>
#include "lexer.h"
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
  char file_name[] = "test.mcc";

  FILE *file;
  file = fopen("test.mcc", "r");

  if(!file){
    printf("Couldn't find file: %s", file_name);
  }
  token *tokens = lexer(file);

  for (size_t i = 0; i < vector_size(tokens); i++) {
    printf("%s\n", tokens[i].value);
  }

  fclose(file);

  return 0;
}
