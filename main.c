#include <ctype.h>
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
 * Split words by whitespace (So checking keywords is easier, and variable names can have numbers)
 * For every character, check if it's a punctuation, number, word, or character
 * If starts with letter, check every keyword then just act like it's a random word
 * Grab the full number for number
 * Tokenize punctuation as-is
 */

typedef enum { SEMI_COLON, OPEN_PARENTHESES, CLOSE_PARENTHESES } type_separator;

typedef enum {
  EXIT,
} type_keyword;

typedef enum {
  INT,
} type_literal;

typedef struct {
  type_separator type;
} token_separator;

typedef struct {
  type_keyword type;
} token_keyword;

typedef struct {
  type_literal type;
  int value;
} token_literal;

token_literal generate_number(char current, FILE *file){
  token_literal token;
  token.type = INT;
  char *value = malloc(sizeof(char) * 8);

  while(isdigit(current) && current != EOF){
    printf("%c", current);
    current = fgetc(file);
  }

  free(value);

  return token;
}

void lexer(FILE *file) {
  char current = fgetc(file);

  while (current != EOF) {
    switch (current) {
    case '(':
      printf("FOUND OPEN PARENTHESES\n");
      break;
    case ')':
      printf("FOUND CLOSED PARENTHESES\n");
      break;
    case ';':
      printf("FOUND SEMICOLON\n");
      break;
    default:
      if (isdigit(current)) {
        printf("FOUND DIGIT: %d\n", current - '0');
      } else if (isalpha(current)) {
        printf("FOUND CHARACTER: %c\n", current);
      }
      break;
    }
    current = fgetc(file);
  }
}

int main(void) {
  FILE *file;
  file = fopen("test.mcc", "r");
  lexer(file);

  return 0;
}
