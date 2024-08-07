#include <ctype.h>
//#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Here's all the things we need to accomplish:
 * Program-wise:
 * Don't use a whole lot of ram at once
 * Optimize bigger operations
 * Don't use a lot of storage
 * Try to reuse variables
 *
 * Tokenizer:
 * Split words by whitespace (So checking keywords is easier, and variable names
 * can have numbers) For every character, check if it's a punctuation, number,
 * word, or character If starts with letter, check every keyword then just act
 * like it's a random word Grab the full number for number Tokenize punctuation
 * as-is
 */

#define tokens_size 128

typedef enum {
  INT,
  KEYWORD,
  SEPARATOR,
} token_type;

typedef struct {
  token_type type;
  char *value;
} token;

token *generate_separator(char current) {
  token *token = malloc(sizeof(token));
  token->type = KEYWORD;
  token->value = malloc(sizeof(char) * 2);
  token->value[0] = current;
  token->value[1] = '\0';

  return token;
}

token *generate_keyword(char current, FILE *file) {
  char *value = malloc(sizeof(char) * 8);
  int value_index = 0;

  while (isalpha(current) && current != EOF) {
    value[value_index] = current;
    value_index++;
    current = fgetc(file);
  };
  // Return character that is not letter
  ungetc(current, file);

  token *token = malloc(sizeof(token));
  token->type = KEYWORD;
  token->value = value;

  return token;
}

token *generate_number(char current, FILE *file) {
  char *value = malloc(sizeof(char) * 8);
  int value_index = 0;

  while (isdigit(current) && current != EOF) {
    value[value_index] = current;
    value_index++;
    current = fgetc(file);
  };
  // Return character that is not number
  ungetc(current, file);

  token *token = malloc(sizeof(token));
  token->type = INT;
  token->value = value;

  return token;
}

token **lexer(FILE *file) {
  // Array of references
  token **tokens = malloc(sizeof(token) * 12);
  int tokens_index = 0;
  token *current_token;

  char current = fgetc(file);

  while (current != EOF) {
    printf("%c\n", current);
    switch (current) {
    case '(':
    case ')':
    case ';':
      current_token = generate_separator(current);
      break;
    default:
      if (isdigit(current)) {
        current_token = generate_number(current, file);
      } else if (isalpha(current)) {
        current_token = generate_keyword(current, file);
      } else {
        printf("UNKNOWN CHARACTER: %c\n", current);
        exit(1);
      }
      break;
    }

    tokens[tokens_index] = current_token;
    tokens_index++;

    current = fgetc(file);
  }

  return tokens;
}

int main(void) {
  FILE *file;
  file = fopen("test.mcc", "r");
  token **tokens = lexer(file);
  fclose(file);

  // For every token, print it's value out
  /*
  int token_index = 0;
  token *token = tokens[token_index];
  while(token != NULL){
    printf("%s\n", token->value);
    token = tokens[token_index];
    token_index++;
  }
  */

  return 0;
}
