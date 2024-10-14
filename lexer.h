#ifndef lexer_h
#define lexer_h
#include <stdio.h>

#define ITERATE_TOKENS_AND(X) \
  X(TOKEN_END) \
  X(TOKEN_BOOL) \
  X(TOKEN_INT) \
  X(TOKEN_CHAR) \
  X(TOKEN_VOID) \
  \
  X(TOKEN_VOLATILE) \
  X(TOKEN_UNSIGNED) \
  X(TOKEN_CONST) \
  X(TOKEN_REGISTER) \
  \
  X(TOKEN_TRUE) \
  X(TOKEN_FALSE) \
  X(TOKEN_NULL) \
  \
  X(TOKEN_SIZEOF) \
  X(TOKEN_TYPEOF) \
  \
  X(TOKEN_STRUCT) \
  X(TOKEN_ENUM) \
  X(TOKEN_UNION) \
  \
  X(TOKEN_DO) \
  X(TOKEN_WHILE) \
  X(TOKEN_FOR) \
  X(TOKEN_BREAK) \
  X(TOKEN_CONTINUE) \
  \
  X(TOKEN_IF) \
  X(TOKEN_ELSE) \
  X(TOKEN_SWITCH) \
  X(TOKEN_CASE) \
  X(TOKEN_DEFAULT) \
  \
  X(TOKEN_INLINE) \
  \
  \
  X(TOKEN_NUMBER) \
  X(TOKEN_STRING) \
  X(TOKEN_TYPE) \
  X(TOKEN_NAME) \
  \
  X(TOKEN_EQUALS) \
  X(TOKEN_EQUALS_EQUALS) \
  X(TOKEN_NOT) \
  X(TOKEN_NOT_EQUALS) \
  \
  X(TOKEN_LESS_THAN) \
  X(TOKEN_LESS_THAN_EQUALS) \
  X(TOKEN_GREATER_THAN) \
  X(TOKEN_GREATER_THAN_EQUALS) \
  \
  X(TOKEN_AMPERSAND) \
  X(TOKEN_AND) \
  X(TOKEN_PIPE) \
  X(TOKEN_OR) \
  X(TOKEN_CARET) \
  \
  X(TOKEN_PLUS) \
  X(TOKEN_PLUS_PLUS) \
  X(TOKEN_MINUS) \
  X(TOKEN_MINUS_MINUS) \
  X(TOKEN_STAR) \
  X(TOKEN_SLASH) \
  \
  X(TOKEN_DOT) \
  X(TOKEN_COMMA) \
  X(TOKEN_SEMI_COLON) \
  X(TOKEN_COLON) \
  X(TOKEN_LEFT_PARENTHESES) \
  X(TOKEN_RIGHT_PARENTHESES) \
  X(TOKEN_LEFT_BRACKET) \
  X(TOKEN_RIGHT_BRACKET) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
  ITERATE_TOKENS_AND(GENERATE_ENUM)
} token_type;

const char *token_type_strings[] = {
  ITERATE_TOKENS_AND(GENERATE_STRING)
};

const char* token_type_to_string(token_type type) {
    return token_type_strings[type];
}

typedef struct {
  token_type type;
  char *value; // Always a vector!
} token;

token *lexer(FILE *file);

#endif 
