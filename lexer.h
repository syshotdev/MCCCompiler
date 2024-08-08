#ifndef lexer_h
#define lexer_h
#include <stdio.h>

typedef enum {
  INT,
  KEYWORD,
  SEPARATOR,
} token_type;

typedef struct {
  token_type type;
  char *value; // Always a vector!
} token;

token *generate_separator(char current);
token *generate_keyword(char current, FILE *file);
token *generate_number(char current, FILE *file);
token *lexer(FILE *file);

#endif 
