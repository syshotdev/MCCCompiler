// Inputs = c source file, outputs = tokens like TOKEN_ASSIGN and TOKEN_EQUALS
// It would be nice if I had some functions like the ones in
// https://craftinginterpreters.com/scanning.html The problem: From what I've
// learned, being object-oriented kinda sucks. So I'll just do what I do.
// Also TODO: Lexer, Preprocessor, Parser, Code_Analyzer.c files

#include "lexer.h"
#include "c-vector/vec.h"
#include <ctype.h>
#include <stdlib.h>

void advance(FILE *file) { fgetc(file); }

char peek(FILE *file) {
  char current = fgetc(file);
  ungetc(current, file);
  return current;
}

bool match(char expected, FILE *file) {
  bool is_same = peek(file) == expected;
  if (is_same) { advance(file); }
  return is_same;
}

token *generate_token(token_type type, char *value) {
  token *token = malloc(sizeof(token));
  token->type = type;
  token->value = value;
  return token;
}

token *generate_punctuation(char current) {
  char *char_vector = vector_create();
  vector_reserve(&char_vector, 2);
  char_vector[0] = current;
  char_vector[1] = '\0';

  return generate_token(TOKEN_PUNCTUATION, char_vector);
}

// Current char is '=' or something, if next char == expected then do compound
// type. Otherwise, single type.
token *generate_math(char expected, FILE *file, token_type single_type,
                     token_type compound_type) {
  token_type type;
  if (peek(file) == expected) {
    type = compound_type;
    advance(file); // 2 chars, skip this one
  } else {
    type = single_type;
  }
  return generate_token(type, vector_create());
}
token *generate_string(FILE *file) {
  char *string_vector = vector_create();
  vector_reserve(&string_vector, 8);

  char current = fgetc(file);
  while (current != '"' && current != EOF) {
    vector_add(&string_vector, current);
    current = fgetc(file);
  }
  return generate_token(TOKEN_STRING, string_vector);
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

  token *token = generate_token(TOKEN_KEYWORD, keyword_vector);

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

  token *token = generate_token(TOKEN_INT, number_vector);

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
    case '\n':
      continue;
      break; // I don't know if this necessary

      // Punctuation
    case '(':
    case ')':
    case '{':
    case '}':
    case '.':
    case ';':
    case ':':
      current_token = generate_punctuation(current);
      break;

      // Math
    case '=':
      current_token =
          generate_math('=', file, TOKEN_EQUALS, TOKEN_EQUALS_EQUALS);
    case '<':
      current_token =
          generate_math('=', file, TOKEN_LESS_THAN, TOKEN_LESS_THAN_EQUALS);
    case '>':
      current_token = generate_math('=', file, TOKEN_GREATER_THAN,
                                    TOKEN_GREATER_THAN_EQUALS);
    case '!':
      current_token = generate_math('=', file, TOKEN_NOT, TOKEN_NOT_EQUALS);
    case '+': 
      // Probably could compact this. Also, should I include +=? 
      // A helper function is what I need for this case section.
      if (match('+', file)){
        current_token = generate_token(TOKEN_PLUS_PLUS, vector_create());
      } else {
        current_token = generate_token(TOKEN_PLUS, vector_create());
      }
      break;
    case '*':
    case '/': // If '//', do comment
    case '%':
    case '&':
    case '|':
    case '^':
      break;

      // Special
    case '\"':
    case '\'':
    case '\\':
      break;

      // Numbers/Keywords
    default:
      if (isdigit(current)) {
        current_token = generate_number(current, file);
      } else if (isalpha(current)) {
        current_token = generate_keyword(current, file);
      } else {
        // This text annoying as heck
        printf("UNKNOWN CHARACTER: %c\n", current);
        continue;
      }
      break;
    }

    printf("What's inside: %s\n", current_token->value);
    vector_add(&tokens, *current_token);
  }

  return tokens;
}
