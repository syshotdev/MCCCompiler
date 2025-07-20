// Inputs = c source char_pointer, outputs = tokens like TOKEN_EQUALS or TOKEN_STRUCT
#include "c-tests/test.h"
#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const char *token_type_strings[] = {ITERATE_TOKENS_AND(GENERATE_STRING)};

const char *token_type_to_string(token_type type) {
  return enum_to_string(type, token_type_strings);
}

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

token create_token(token_type type) {
  token current_token = {
    .type = type,
    .value = vector_create(),
  };
  return current_token;
}

// Advance till a specific string of characters is found (stops before pointer
// is on string)
void advance_till(char terminating_string[], char **char_pointer) {
  char_vector current_chars = vector_create();
  int terminating_string_length = strlen(terminating_string);
  vector_resize(&current_chars, terminating_string_length);
  while (true) {
    char current_char = pop_char(char_pointer);

    if ((int)vector_size((vector *)&current_chars) == terminating_string_length) {
      vector_remove(&current_chars, 0);
    }
    vector_add(&current_chars, current_char);

    bool is_terminating_string = strncmp(current_chars, terminating_string, terminating_string_length) == 0;
    if (is_terminating_string || current_char == '\0') {
      return;
    }
  }
}

// Make a string till condition is false, at end finish string and do misc
// String addr is required as we are reallocating the string, which makes the old string pointer useless.
void collect_characters(char_vector *string_addr, bool (*condition)(char), char **char_pointer) {
  char current_char = pop_char(char_pointer);
  while (condition(current_char) && current_char != EOF) {
    vector_add(string_addr, current_char);
    current_char = pop_char(char_pointer);
  }
  // Return consumed char
  return_char(char_pointer);
  // Add string ending notifier
  vector_add(string_addr, '\0');
}

token create_single(token_type type, char **char_pointer) {
  advance_char(char_pointer);
  token current_token = create_token(type);
  vector_add(&current_token.value, '\0');
  return current_token;
}

// Generate token like = or == (or <=) (This definition sucks)
// We are at first char. If next char is expected, create twice token, otherwise
// create once. Ex: We are at '-', if we advance char and another '-', create
// twice token.
token create_double(char expected, token_type once, token_type twice, char **char_pointer) {
  // Skip current "once" char, and check if next char is "twice" chars
  advance_char(char_pointer);

  token current_token = create_token(TOKEN_END);
  vector_add(&current_token.value, '\0');
  bool is_expected = peek_char(char_pointer) == expected;
  if (is_expected) {
    // Skip "twice" char
    advance_char(char_pointer);
    current_token.type = twice;
    return current_token;
  } else {
    current_token.type = once;
    return current_token;
  }
}

bool not_quote(char c) { return c != '"'; }

token create_string(char **char_pointer) {
  token string_token = create_token(TOKEN_STRING);
  vector_resize(&string_token.value, 8);

  advance_char(char_pointer); // Remove "
  collect_characters(&string_token.value, not_quote, char_pointer);
  advance_char(char_pointer); // Remove "

  return string_token;
}

bool isdigit_wrapper(char c) { return isdigit((unsigned char)c) != 0; }

token create_number(char **char_pointer) {
  token number_token = create_token(TOKEN_NUMBER);
  vector_resize(&number_token.value, 8);

  collect_characters(&number_token.value, isdigit_wrapper, char_pointer);

  return number_token;
}

// Keywords/variables can only have these characters
bool is_valid_keyword_char(char to_check) {
  return isalpha(to_check) || isdigit(to_check) || to_check == '_';
}

// Shortcut for strncmp
bool vector_is(char_vector value, char *to_compare) {
  // Strings with different sizes are, by definition, different
  if (vector_size((vector*)&value) - 1 != strlen(to_compare)) {
    return false;
  }
  // Compare strings with size as a restraint
  return strncmp(value, to_compare, vector_size((vector*)&value)) == 0;
}

token create_keyword(char **char_pointer) {
  token keyword_token = create_token(TOKEN_NAME);

  // It's a miracle that this works
  collect_characters(&keyword_token.value, is_valid_keyword_char, char_pointer);

  char_vector value = keyword_token.value;
  token_type type;

  // TODO: Turn this into hashmap (or switch)
  if (vector_is(value, "volatile")) {
    type = TOKEN_VOLATILE;
  } else if (vector_is(value, "unsigned")) {
    type = TOKEN_UNSIGNED;
  } else if (vector_is(value, "const")) {
    type = TOKEN_CONST;
  } else if (vector_is(value, "register")) {
    type = TOKEN_REGISTER;
  } else if (vector_is(value, "true")) {
    type = TOKEN_TRUE;
  } else if (vector_is(value, "false")) {
    type = TOKEN_FALSE;
  } else if (vector_is(value, "null")) {
    type = TOKEN_NULL;
  } else if (vector_is(value, "sizeof")) {
    type = TOKEN_SIZEOF;
  } else if (vector_is(value, "typeof")) {
    type = TOKEN_TYPEOF;
  } else if (vector_is(value, "typedef")) {
    type = TOKEN_TYPEDEF;
  } else if (vector_is(value, "struct")) {
    type = TOKEN_STRUCT;
  } else if (vector_is(value, "enum")) {
    type = TOKEN_ENUM;
  } else if (vector_is(value, "union")) {
    type = TOKEN_UNION;
  } else if (vector_is(value, "do")) {
    type = TOKEN_DO;
  } else if (vector_is(value, "while")) {
    type = TOKEN_WHILE;
  } else if (vector_is(value, "for")) {
    type = TOKEN_FOR;
  } else if (vector_is(value, "break")) {
    type = TOKEN_BREAK;
  } else if (vector_is(value, "continue")) {
    type = TOKEN_CONTINUE;
  } else if (vector_is(value, "if")) {
    type = TOKEN_IF;
  } else if (vector_is(value, "else")) {
    type = TOKEN_ELSE;
  } else if (vector_is(value, "switch")) {
    type = TOKEN_SWITCH;
  } else if (vector_is(value, "case")) {
    type = TOKEN_CASE;
  } else if (vector_is(value, "default")) {
    type = TOKEN_DEFAULT;
  } else if (vector_is(value, "inline")) {
    type = TOKEN_INLINE;
  } else {
    type = TOKEN_NAME;
  }

  keyword_token.type = type;

  return keyword_token;
}

token *lexer(char_vector chars) {
  char **char_pointer = &chars;

  // Make a vector of pointers so I can avoid memory leaks where I copy
  // token struct and leak the one originally pointed to
  token *tokens = vector_create();
  vector_resize(&tokens, 16);
  token current_token;

  char current_char;
  // Note: Since we only peek_char here, it's the function's responsibility to pop the char
  // and keep the loop not... infinite.
  while (true) {
    current_char = peek_char(char_pointer);
    if (current_char == EOF || current_char == '\0') {
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
        printf("UNKNOWN CHARACTER: %c\n", current_char);
        // Unknown character? Skip it.
        advance_char(char_pointer);
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
      current_token = create_double('=', TOKEN_EQUALS, TOKEN_EQUALS_EQUALS, char_pointer);
      break;
    case '!':
      current_token = create_double('=', TOKEN_NOT, TOKEN_NOT_EQUALS, char_pointer);
      break;
    case '<':
      current_token = create_double('=', TOKEN_LESS_THAN, TOKEN_LESS_THAN_EQUALS, char_pointer);
      break;
    case '>':
      current_token = create_double('=', TOKEN_GREATER_THAN, TOKEN_GREATER_THAN_EQUALS, char_pointer);
      break;
    case '&':
      current_token = create_double('&', TOKEN_AMPERSAND, TOKEN_AND, char_pointer);
      break;
    case '|':
      current_token = create_double('|', TOKEN_PIPE, TOKEN_OR, char_pointer);
      break;
    case '^':
      current_token = create_single(TOKEN_CARET, char_pointer);
      break;

    // Math operations
    case '+':
      advance_char(char_pointer);
      switch (peek_char(char_pointer)) {
      case '+':
        current_token = create_single(TOKEN_PLUS_PLUS, char_pointer);
        break;
      case '=':
        current_token = create_single(TOKEN_PLUS_EQUALS, char_pointer);
        break;
      default:
        // We create token here because '+' is already skipped
        current_token = create_token(TOKEN_PLUS);
        break;
      }
      break;
    case '-':
      advance_char(char_pointer);
      switch (peek_char(char_pointer)) {
      case '-':
        current_token = create_single(TOKEN_MINUS_MINUS, char_pointer);
        break;
      case '=':
        current_token = create_single(TOKEN_MINUS_EQUALS, char_pointer);
        break;
      case '>':
        current_token = create_single(TOKEN_ARROW, char_pointer);
        break;
      default:
        // We create token here because '+' is already skipped
        current_token = create_token(TOKEN_MINUS);
        break;
      }
      break;
    case '*':
      current_token = create_single(TOKEN_STAR, char_pointer);
      break;
    case '/':
      // We get rid of first '/' and peek next one
      pop_char(char_pointer);
      switch (peek_char(char_pointer)) {
      case '/':
        advance_till("\n", char_pointer);
        continue;
        break;
      case '*':
        advance_till("*/", char_pointer);
        continue;
        break;
      default:
        // We create token here because '/' is already skipped
        current_token = create_token(TOKEN_SLASH);
        break;
      }
      break;
    case '%':
      current_token = create_single(TOKEN_PERCENT, char_pointer);
      break;

    // Punctuation
    case '(':
      current_token = create_single(TOKEN_LEFT_PARENTHESES, char_pointer);
      break;
    case ')':
      current_token = create_single(TOKEN_RIGHT_PARENTHESES, char_pointer);
      break;
    case '[':
      current_token = create_single(TOKEN_LEFT_BRACKET, char_pointer);
      break;
    case ']':
      current_token = create_single(TOKEN_RIGHT_BRACKET, char_pointer);
      break;
    case '{':
      current_token = create_single(TOKEN_LEFT_BRACE, char_pointer);
      break;
    case '}':
      current_token = create_single(TOKEN_RIGHT_BRACE, char_pointer);
      break;
    case '.':
      current_token = create_single(TOKEN_DOT, char_pointer);
      break;
    case ',':
      current_token = create_single(TOKEN_COMMA, char_pointer);
      break;
    case ';':
      current_token = create_single(TOKEN_SEMI_COLON, char_pointer);
      break;
    case ':':
      current_token = create_single(TOKEN_COLON, char_pointer);
      break;
    }

    printf("What's inside: %s\n", token_type_to_string(current_token.type));
    vector_add(&tokens, current_token);
    assert(vector_size((vector*)&current_token.value) > 0);
    char char_at_end_of_string = *(char*)vector_get((vector*)&current_token.value, vector_size((vector*)&current_token.value) - 1);
    assert(char_at_end_of_string == '\0');
  }
  vector_add(&tokens, create_token(TOKEN_END));

  return tokens;
}
