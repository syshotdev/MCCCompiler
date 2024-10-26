// Inputs = c source file, outputs = tokens like TOKEN_EQUALS or TOKEN_STRUCT
// Also TODO: Lexer, Preprocessor, Parser, Code_Analyzer.c files

#include "lexer.h"
#include "c-vector/vec.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const char *token_type_strings[] = {ITERATE_TOKENS_AND(GENERATE_STRING)};

const char *token_type_to_string(token_type type) {
  return enum_to_string(type, token_type_strings);
}

// Pop the next character
char pop_char(FILE *file) { return fgetc(file); }

// Get the next character without advancing
char peek_char(FILE *file) {
  char current = fgetc(file);
  ungetc(current, file);
  return current;
}

// Return whether the next character == expected, and if expected pop char
bool match_char(char expected, FILE *file) {
  bool is_same = peek_char(file) == expected;
  if (is_same) {
    pop_char(file);
  }
  return is_same;
}

// Go backwards in characters
void return_char(char character, FILE *file) { ungetc(character, file); }

// Not implemented yet. For when I build my own standard library
void advance_char(FILE file);

// Advance till a specific string of characters is found
void advance_till(char terminating_string[], FILE *file) {
  char *current_chars = vector_create();
  int terminating_string_length = strlen(terminating_string);
  vector_reserve(&current_chars, terminating_string_length);
  while (true) {
    char current_char = pop_char(file);

    if ((int)vector_size(current_chars) == terminating_string_length) {
      vector_remove(&current_chars, 0);
    }
    vector_add(&current_chars, current_char);

    bool is_terminating_string = strcmp(current_chars, terminating_string) == 0;
    if (is_terminating_string || EOF) {
      return;
    }
  }
}

token *create_token(token_type type) {
  token *token = malloc(sizeof(token));
  token->type = type;
  token->value = vector_create();
  return token;
}

// Generate token like = or == (or <=)
token *create_double(char expected, token_type once, token_type twice,
                     FILE *file) {
  if (match_char(expected, file)) {
    return create_token(twice);
  } else {
    return create_token(once);
  }
}

// Make a string till condition is false, at end finish string and do misc
void collect_characters(token *token, bool (*condition)(char), FILE *file) {
  char current = pop_char(file);
  while (condition(current) && current != EOF) {
    vector_add(&token->value, current);
    current = pop_char(file);
  }
  // Return consumed char
  return_char(current, file);
  // Add string ending notifier
  vector_add(&token->value, '\0');
}

bool not_quote(char c) { return c != '"'; }

token *create_string(FILE *file) {
  token *string_token = create_token(TOKEN_STRING);
  vector_reserve(&string_token->value, 8);

  // " was skipped from main loop
  collect_characters(string_token, not_quote, file);
  pop_char(file); // Remove extra "

  return string_token;
}

bool isdigit_wrapper(char c) { return isdigit((unsigned char)c) != 0; }

token *create_number(char current, FILE *file) {
  token *number_token = create_token(TOKEN_NUMBER);
  vector_reserve(&number_token->value, 8);

  return_char(current, file);
  collect_characters(number_token, isdigit_wrapper, file);

  return number_token;
}

// Keywords/variables can only have these characters
bool is_valid_keyword_char(char to_check) {
  return isalpha(to_check) || isdigit(to_check) || to_check == '_';
}

// More readable version of strcmp
bool value_is(char *value, char *to_compare) {
  return strcmp(value, to_compare) == 0;
}

token *create_keyword(char current, FILE *file) {
  token *keyword_token = create_token(TOKEN_NAME);

  return_char(current, file);
  collect_characters(keyword_token, is_valid_keyword_char, file);

  char *value = keyword_token->value;
  token_type type;

  // TODO: Turn this into hashmap (or switch)
  if (value_is(value, "volatile")) {
    type = TOKEN_VOLATILE;
  } else if (value_is(value, "unsigned")) {
    type = TOKEN_UNSIGNED;
  } else if (value_is(value, "const")) {
    type = TOKEN_CONST;
  } else if (value_is(value, "register")) {
    type = TOKEN_REGISTER;
  } else if (value_is(value, "true")) {
    type = TOKEN_TRUE;
  } else if (value_is(value, "false")) {
    type = TOKEN_FALSE;
  } else if (value_is(value, "null")) {
    type = TOKEN_NULL;
  } else if (value_is(value, "sizeof")) {
    type = TOKEN_SIZEOF;
  } else if (value_is(value, "typeof")) {
    type = TOKEN_TYPEOF;
  } else if (value_is(value, "struct")) {
    type = TOKEN_STRUCT;
  } else if (value_is(value, "enum")) {
    type = TOKEN_ENUM;
  } else if (value_is(value, "union")) {
    type = TOKEN_UNION;
  } else if (value_is(value, "do")) {
    type = TOKEN_DO;
  } else if (value_is(value, "while")) {
    type = TOKEN_WHILE;
  } else if (value_is(value, "for")) {
    type = TOKEN_FOR;
  } else if (value_is(value, "break")) {
    type = TOKEN_BREAK;
  } else if (value_is(value, "continue")) {
    type = TOKEN_CONTINUE;
  } else if (value_is(value, "if")) {
    type = TOKEN_IF;
  } else if (value_is(value, "else")) {
    type = TOKEN_ELSE;
  } else if (value_is(value, "switch")) {
    type = TOKEN_SWITCH;
  } else if (value_is(value, "case")) {
    type = TOKEN_CASE;
  } else if (value_is(value, "default")) {
    type = TOKEN_DEFAULT;
  } else if (value_is(value, "inline")) {
    type = TOKEN_INLINE;
  } else {
    type = TOKEN_NAME;
  }

  keyword_token->type = type;

  return keyword_token;
}

token *lexer(FILE *file) {
  // Make a vector of pointers so I can avoid memory leaks where I copy
  // dereferenced token struct and leak the one pointed to
  token *tokens = vector_create();
  vector_reserve(&tokens, 16);
  token *current_token;

  char current_char;
  while (true) {
    current_char = pop_char(file);
    if (current_char == EOF) {
      break;
    }
    switch (current_char) {
    // Skip whitespace
    case ' ':
    case '\t':
    case '\n':
      continue;

    // Numbers/Keywords
    default:
      if (isdigit(current_char)) {
        current_token = create_number(current_char, file);
      } else if (isalpha(current_char)) {
        current_token = create_keyword(current_char, file);
      } else {
        // This text annoying as heck
        printf("UNKNOWN CHARACTER: %c\n", current_char);
        continue;
      }
      break;

    // Special
    case '\"':
      current_token = create_string(file);
      break;
    case '\'':
      // TODO: create_char
      break;
    case '\\':
      // Man this is too complex for now
      break;

    // Equality
    case '=':
      current_token =
          create_double('=', TOKEN_EQUALS, TOKEN_EQUALS_EQUALS, file);
      break;
    case '!':
      current_token = create_double('=', TOKEN_NOT, TOKEN_NOT_EQUALS, file);
      break;
    case '<':
      current_token =
          create_double('=', TOKEN_LESS_THAN, TOKEN_LESS_THAN_EQUALS, file);
      break;
    case '>':
      current_token = create_double('=', TOKEN_GREATER_THAN,
                                    TOKEN_GREATER_THAN_EQUALS, file);
      break;
    case '&':
      current_token = create_double('&', TOKEN_AMPERSAND, TOKEN_AND, file);
      break;
    case '|':
      current_token = create_double('|', TOKEN_PIPE, TOKEN_OR, file);
      break;
    case '^':
      current_token = create_token(TOKEN_CARET);
      break;

    // Math operations
    case '+':
      current_token = create_double('+', TOKEN_PLUS, TOKEN_PLUS_PLUS, file);
      break;
    case '-':
      // Wish I could do ->
      current_token = create_double('+', TOKEN_MINUS, TOKEN_MINUS_MINUS, file);
      break;
    case '*':
      current_token = create_token(TOKEN_STAR);
      break;
    case '/':
      switch (peek_char(file)) {
      case '/':
        pop_char(file);
        advance_till("\n", file);
        break;
      case '*':
        pop_char(file);
        advance_till("*/", file);
        break;
      default:
        current_token = create_token(TOKEN_SLASH);
        break;
      }
      break;
    case '%':
      current_token = create_token(TOKEN_PERCENT);
      break;

    // Punctuation
    case '(':
      current_token = create_token(TOKEN_LEFT_PARENTHESES);
      break;
    case ')':
      current_token = create_token(TOKEN_RIGHT_PARENTHESES);
      break;
    case '[':
      current_token = create_token(TOKEN_LEFT_BRACKET);
      break;
    case ']':
      current_token = create_token(TOKEN_RIGHT_BRACKET);
      break;
    case '{':
      current_token = create_token(TOKEN_LEFT_BRACKET);
      break;
    case '}':
      current_token = create_token(TOKEN_RIGHT_BRACKET);
      break;
    case '.':
      current_token = create_token(TOKEN_DOT);
      break;
    case ',':
      current_token = create_token(TOKEN_COMMA);
      break;
    case ';':
      current_token = create_token(TOKEN_SEMI_COLON);
      break;
    case ':':
      current_token = create_token(TOKEN_COLON);
      break;
    }

    printf("What's inside: %s\n", token_type_to_string(current_token->type));
    vector_add(&tokens, *current_token);
  }

  return tokens;
}
