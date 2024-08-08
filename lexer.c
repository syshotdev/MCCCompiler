#include "c-vector/vec.h"
#include <ctype.h>
#include "lexer.h"
#include <stdlib.h>

token *generate_separator(char current) {
  token *token = malloc(sizeof(token));
  token->type = KEYWORD;
  token->value = vector_create();
  vector_reserve(&token->value, 2);

  token->value[0] = current;
  token->value[1] = '\0';

  return token;
}

token *generate_keyword(char current, FILE *file) {
  char *keyword_vector = vector_create();
  vector_reserve(&keyword_vector, 8);

  while (isalpha(current) && current != EOF) {
    vector_add(&keyword_vector, current);
    current = fgetc(file);
  };
  // Return character that is not letter
  ungetc(current, file);

  token *token = malloc(sizeof(token));
  token->type = KEYWORD;
  token->value = keyword_vector;

  return token;
}

token *generate_number(char current, FILE *file) {
  char *number_vector = vector_create();
  vector_reserve(&number_vector, 8);

  while (isdigit(current) && current != EOF) {
    vector_add(&number_vector, current);
    current = fgetc(file);
  };
  // Return character that is not number
  ungetc(current, file);

  token *token = malloc(sizeof(token));
  token->type = INT;
  token->value = number_vector;

  return token;
}

// printf("%s\n", token->value);
token *lexer(FILE *file) {
  // Array of references to the tokens
  token *tokens = vector_create();
  vector_reserve(&tokens, 16);
  token *current_token;

  char current = ' ';
  while (current != EOF) {
    current = fgetc(file);
    switch (current) {
    // Skip whitespace
    case ' ':
    case '\t':
      continue;
      break; // I don't know if this necessary
    case '(':
    case ')':
    case '{':
    case '}':
    case '.':
    case ';':
      current_token = generate_separator(current);
      break;
    default:
      if (isdigit(current)) {
        current_token = generate_number(current, file);
      } else if (isalpha(current)) {
        current_token = generate_keyword(current, file);
      } else {
        // This text annoying as heck
        printf("UNKNOWN CHARACTER: %c\n", current);
        continue; // exit(1);
      }
      break;
    }

    printf("What's inside: %s\n", current_token->value);
    vector_add(&tokens, *current_token);
  }

  return tokens;
}
