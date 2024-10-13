#ifndef lexer_h
#define lexer_h
#include <stdio.h>

typedef enum {
  TOKEN_END,

  TOKEN_INT,
  TOKEN_STRING,
  TOKEN_NAME, // Function name, variable name

  TOKEN_KEYWORD,
  TOKEN_SEPARATOR,
  TOKEN_RETURN,

  // TODO: +=, Keywords like return, while, for, stuff like that
  TOKEN_EQUALS,
  /*
  TOKEN_EQUALS_EQUALS,
  TOKEN_LESS_THAN,
  TOKEN_LESS_THAN_EQUALS,
  TOKEN_GREATER_THAN,
  TOKEN_GREATER_THAN_EQUALS,
  TOKEN_NOT,
  TOKEN_NOT_EQUALS,

  TOKEN_PLUS_EQUALS,
  TOKEN_MINUS_EQUALS,
  TOKEN_STAR_EQUALS,
  TOKEN_SLASH_EQUALS,
  TOKEN_PERCENT_EQUALS,
  */

  TOKEN_PLUS,
  TOKEN_PLUS_PLUS,
  TOKEN_MINUS,
  TOKEN_MINUS_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_PERCENT,

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
