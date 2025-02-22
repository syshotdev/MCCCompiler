// Inputs = c source char_pointer, outputs = tokens like TOKEN_EQUALS or
// TOKEN_STRUCT Also TODO: Lexer, Preprocessor, Parser, Code_Analyzer.c
#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const char *token_type_strings[] = {ITERATE_TOKENS_AND(GENERATE_STRING)};

const char *token_type_to_string(token_type type) {
  return enum_to_string(type, token_type_strings);
}

// Not implemented yet. For when I build my own standard library
void advance_char(char **char_pointer) {
  // Pointer arithmetic to advance pointer by 1 char length
  *char_pointer += 1;
}
void return_char(char **char_pointer) {
  // Pointer arithmetic to de-advance pointer by 1 char length
  *char_pointer -= 1;
}

char pop_char(char **char_pointer) {
  char current_char = **char_pointer;
  advance_char(char_pointer);
  return current_char;
}

char peek_char(char **char_pointer) {
  char current_char = **char_pointer;
  return current_char;
}

// Return whether the next character == expected, and if expected pop char
bool match_char(char expected, char **char_pointer) {
  bool is_same = peek_char(char_pointer) == expected;
  if (is_same) {
    advance_char(char_pointer);
  }
  return is_same;
}

// Advance till a specific string of characters is found
void advance_till(char terminating_string[], char **char_pointer) {
  char_vector current_chars = vector_create();
  int terminating_string_length = strlen(terminating_string);
  vector_resize(&current_chars, terminating_string_length);
  while (true) {
    char current_char = pop_char(char_pointer);

    if ((int)vector_size(current_chars) == terminating_string_length) {
      vector_remove(&current_chars, 0);
    }
    vector_add(&current_chars, current_char);

    bool is_terminating_string = strcmp(current_chars, terminating_string) == 0;
    if (is_terminating_string || current_char == EOF) {
      return;
    }
  }
}

// Make a string till condition is false, at end finish string and do misc
void collect_characters(token *token, bool (*condition)(char),
                        char **char_pointer) {
  char current_char = pop_char(char_pointer);
  while (condition(current_char) && current_char != EOF) {
    vector_add(&token->value, current_char);
    current_char = pop_char(char_pointer);
  }
  // Return consumed char
  return_char(char_pointer);
  // Add string ending notifier
  vector_add(&token->value, '\0');
}

token *create_token(token_type type) {
  token *token = malloc(sizeof(token));
  token->type = type;
  token->value = vector_create();
  return token;
}

// Generate token like = or == (or <=)
token *create_double(char expected, token_type once, token_type twice,
                     char **char_pointer) {
  if (match_char(expected, char_pointer)) {
    return create_token(twice);
  } else {
    return create_token(once);
  }
}

bool not_quote(char c) { return c != '"'; }

token *create_string(char **char_pointer) {
  token *string_token = create_token(TOKEN_STRING);
  vector_reserve(&string_token->value, 8);

  // " was skipped from main loop
  collect_characters(string_token, not_quote, char_pointer);
  pop_char(char_pointer); // Remove extra "

  return string_token;
}

bool isdigit_wrapper(char c) { return isdigit((unsigned char)c) != 0; }

token *create_number(char **char_pointer) {
  token *number_token = create_token(TOKEN_NUMBER);
  vector_reserve(&number_token->value, 8);

  collect_characters(number_token, isdigit_wrapper, char_pointer);

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

token *create_keyword(char **char_pointer) {
  token *keyword_token = create_token(TOKEN_NAME);

  collect_characters(keyword_token, is_valid_keyword_char, char_pointer);

  char_vector value = keyword_token->value;
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

token *lexer(char_vector chars) {
  char **char_pointer = &chars;

  // Make a vector of pointers so I can avoid memory leaks where I copy
  // dereferenced token struct and leak the one pointed to
  token *tokens = vector_create();
  vector_reserve(&tokens, 16);
  token *current_token;

  char current_char;
  while (true) {
    current_char = peek_char(char_pointer);
    if (current_char == EOF) {
      break;
    }
    switch (current_char) {
    // Skip whitespace
    case ' ':
    case '\t':
    case '\n':
      advance_char(char_pointer);
      continue;

    // Numbers/Keywords
    default:
      if (isdigit(current_char)) {
        current_token = create_number(char_pointer);
      } else if (isalpha(current_char)) {
        current_token = create_keyword(char_pointer);
      } else {
        // This text annoying as heck
        printf("UNKNOWN CHARACTER: %c\n", current_char);
        continue;
      }
      break;

    // Special
    case '\"':
      current_token = create_string(char_pointer);
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
          create_double('=', TOKEN_EQUALS, TOKEN_EQUALS_EQUALS, char_pointer);
      break;
    case '!':
      current_token =
          create_double('=', TOKEN_NOT, TOKEN_NOT_EQUALS, char_pointer);
      break;
    case '<':
      current_token = create_double('=', TOKEN_LESS_THAN,
                                    TOKEN_LESS_THAN_EQUALS, char_pointer);
      break;
    case '>':
      current_token = create_double('=', TOKEN_GREATER_THAN,
                                    TOKEN_GREATER_THAN_EQUALS, char_pointer);
      break;
    case '&':
      current_token =
          create_double('&', TOKEN_AMPERSAND, TOKEN_AND, char_pointer);
      break;
    case '|':
      current_token = create_double('|', TOKEN_PIPE, TOKEN_OR, char_pointer);
      break;
    case '^':
      current_token = create_token(TOKEN_CARET);
      break;

    // Math operations
    case '+':
      current_token =
          create_double('+', TOKEN_PLUS, TOKEN_PLUS_PLUS, char_pointer);
      break;
    case '-':
      // Wish I could do ->
      current_token =
          create_double('+', TOKEN_MINUS, TOKEN_MINUS_MINUS, char_pointer);
      break;
    case '*':
      current_token = create_token(TOKEN_STAR);
      break;
    case '/':
      switch (peek_char(char_pointer)) {
      case '/':
        pop_char(char_pointer);
        advance_till("\n", char_pointer);
        break;
      case '*':
        pop_char(char_pointer);
        advance_till("*/", char_pointer);
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
