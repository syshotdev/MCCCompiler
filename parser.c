#include "c-tests/test.h"
#include "c-vector/vec.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char *node_type_strings[] = {ITERATE_NODES_AND(GENERATE_STRING)};

const char *node_type_to_string(node_type type) {
  return enum_to_string(type, node_type_strings);
}

void advance_token(token **token_pointer) {
  // Pointer arithmetic to advance pointer by 1 token length
  *token_pointer += 1;
}
void return_token(token **token_pointer) {
  // Pointer arithmetic to de-advance pointer by 1 token length
  *token_pointer -= 1;
}

token *pop_token(token **token_pointer) {
  token *current_token = &(**token_pointer);
  advance_token(token_pointer);
  return current_token;
}

token *peek_token(token **token_pointer) {
  token *current_token = &(**token_pointer);
  return current_token;
}

bool is_at_end(token **token_pointer) {
  return peek_token(token_pointer)->type == TOKEN_END;
}

token *expect_token(token_type type, token **token_pointer) {
  token *token = pop_token(token_pointer);
  if (token->type != type) {
    // Should error
    printf("Token given: %s \n Token expected: %s \n",
           token_type_to_string(token->type), token_type_to_string(type));
  }
  return token;
}

node *create_node(node_type type) {
  node *current_node = malloc(sizeof(node));
  current_node->type = type;
  return current_node;
}

precedence get_precedence(token_type type) {
  precedence precedence = PRECEDENCE_ASSIGNMENT;
  switch (type) {
  default:
    precedence = PRECEDENCE_NONE;
    break;
  case TOKEN_OR:
    precedence = PRECEDENCE_OR;
    break;
  case TOKEN_AND:
    precedence = PRECEDENCE_AND;
    break;
  case TOKEN_EQUALS_EQUALS:
  case TOKEN_NOT_EQUALS:
    precedence = PRECEDENCE_EQUALITY;
    break;
  case TOKEN_LESS_THAN:
  case TOKEN_GREATER_THAN:
  case TOKEN_LESS_THAN_EQUALS:
  case TOKEN_GREATER_THAN_EQUALS:
    precedence = PRECEDENCE_COMPARISON;
    break;
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    precedence = PRECEDENCE_TERM;
    break;
  case TOKEN_STAR:
  case TOKEN_SLASH:
    precedence = PRECEDENCE_FACTOR;
    break;
  case TOKEN_NOT:
    precedence = PRECEDENCE_UNARY;
    break;
  case TOKEN_DOT:
  case TOKEN_LEFT_PARENTHESES:
    precedence = PRECEDENCE_CALL;
    break;
  case TOKEN_NAME:
  case TOKEN_STRING:
  case TOKEN_NUMBER:
    precedence = PRECEDENCE_PRIMARY;
    break;
  }
  return precedence;
}

// Prototype because it complains about multiple function definitions (It's used
// before it's defined).
node *parse_precedence(precedence precedence, token **token_pointer);

node *parse_number(token **token_pointer) {
  node *number_node = create_node(NODE_NUMBER_LITERAL);
  // Get value from that token that sent us here
  int value = atoi(pop_token(token_pointer)->value);
  number_node->number_literal.value = value;

  return number_node;
}

node *parse_grouping(token **token_pointer) {
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  node *ast = parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  return ast;
}

node *parse_binary(node *last_expression, token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  node *next_expression =
      parse_precedence(get_precedence(operator_token->type) + 1, token_pointer);

  operator_type operator;
  switch (operator_token->type) {
  case TOKEN_PLUS:
    operator = OPERATOR_ADD;
    break;
  case TOKEN_MINUS:
    operator = OPERATOR_SUBTRACT;
    break;
  case TOKEN_STAR:
    operator = OPERATOR_MULTIPLY;
    break;
  case TOKEN_SLASH:
    operator = OPERATOR_DIVIDE;
    break;
  default:
    printf("Error in parse binary");
    break;
  }

  node *ast = create_node(NODE_EQUATION);
  ast->equation.operator = operator;
  ast->equation.left = last_expression;
  ast->equation.right = next_expression;

  return ast;
}

node *parse_unary(token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  node *expression = parse_precedence(PRECEDENCE_UNARY, token_pointer);

  operator_type operator;
  switch (operator_token->type) {
  // +NUMBER does nothing
  case TOKEN_PLUS:
    return expression;
    break;
  case TOKEN_MINUS:
    operator = OPERATOR_NEGATE;
    break;
  case TOKEN_NOT:
    operator = OPERATOR_NOT;
    break;
  default:
    printf("Error in parse unary");
    exit(1);
    break;
  }

  node *ast = create_node(NODE_EQUATION);
  ast->equation.operator = operator;
  ast->equation.left = expression;
  ast->equation.right = NULL;

  return ast;
}

node *parse_precedence(precedence precedence, token **token_pointer) {
  token *current_token = peek_token(token_pointer);

  // Unary operators
  switch (current_token->type) {
  default:
    break;
  case TOKEN_MINUS:
  case TOKEN_PLUS:
  case TOKEN_NOT:
    return parse_unary(token_pointer);
    break;
  }

  node *ast = NULL;
  node *last_expression = NULL;
  node *current_expression = NULL;

  // Keep parsing equal or higher precedence values
  // Example: -12 - -6 * (2 + 3);
  while (precedence <= get_precedence(current_token->type)) {
    switch (current_token->type) {
    case TOKEN_SEMI_COLON:
      // Error here, semi colon not supposed to be in the middle of expression.
      // It doesn't make sense, but in "get_precedence", semi colon means "just return, don't loop a last time"
      return ast;

    case TOKEN_LEFT_PARENTHESES:
      current_expression = parse_grouping(token_pointer);
      break;

    case TOKEN_NUMBER:
      current_expression = parse_number(token_pointer);
      break;

    case TOKEN_NAME:
      // Check if variable name or function call
      break;

    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
      current_expression = parse_binary(last_expression, token_pointer);
      break;

    default:
      // TODO: Better error handling
      printf("Error when parsing precedence: Unknown token %s",
             token_type_to_string(current_token->type));
      return ast;
    }
    current_token = peek_token(token_pointer);
    last_expression = current_expression;

    ast = current_expression;
  }

  return ast;
}

// Wrapper for parse_precendense
node *parse_expression(token **token_pointer) {
  node *ast = parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  return ast;
}

int how_many_in_a_row(token_type type, token **token_pointer) {
  int number = 0;
  while (true) {
    if (peek_token(token_pointer)->type != type) {
      break;
    }
    // Pop the token
    pop_token(token_pointer);
    number++;
  }
  return number;
}

// Parse something like "a = 12 + (9 * 20);"
node *parse_variable_assignment(token **token_pointer) {
  node *ast = create_node(NODE_VARIABLE_DECLARATION);
  // TODO: Figure out whether to make new node or roll with 'no type'
  // ast->variable.type = NULL;
  ast->variable.name = expect_token(TOKEN_NAME, token_pointer)->value;
  expect_token(TOKEN_EQUALS, token_pointer);
  ast->variable.value = parse_expression(token_pointer);
  return ast;
}

// Prototype for later lines
node *parse_scope(scope_context *context, int scope, token **token_pointer);

// Parse a variable or function declaration.
node *parse_type(token **token_pointer, scope_context *context, int scope) {
  // Get the type, how many references, then the name of function/variable
  type_info type = {
    .name = expect_token(TOKEN_NAME, token_pointer)->value,
    .pointer_amount = how_many_in_a_row(TOKEN_STAR, token_pointer),
  };
  char_vector name = expect_token(TOKEN_NAME, token_pointer)->value;

  node *ast = create_node(NODE_NONE);
  // Set the type_info and name for variables AND functions AND parameters
  ast->variable.type = type;
  ast->variable.name = name;

  token *current_token = peek_token(token_pointer);
  switch (current_token->type) {
  case TOKEN_EQUALS:
    expect_token(TOKEN_EQUALS, token_pointer);
    ast->type = NODE_VARIABLE_DECLARATION;
    ast->variable.value = parse_expression(token_pointer);
    break;
  case TOKEN_SEMI_COLON:
    expect_token(TOKEN_SEMI_COLON, token_pointer);
    ast->type = NODE_VARIABLE_DECLARATION;
    ast->variable.value = NULL;
    break;
  case TOKEN_LEFT_PARENTHESES:
    expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
    // make vector of parameters and each name add to it
    // Maybe I should use the parse_type function recursively here?
    // Yeah, that's probably smart. Or maybe I put this in it's own function.
    // Basically, it could be a 'struct type varname' instead of just 'type varname'
    if (peek_token(token_pointer)->type == TOKEN_NAME) {
      while(!is_at_end(token_pointer)) {
        // type
        expect_token(TOKEN_NAME, token_pointer);
        // variable name
        expect_token(TOKEN_NAME, token_pointer);
        if (peek_token(token_pointer)->type == TOKEN_RIGHT_PARENTHESES) {
          break;
        }
        // 3 dots (Idk how this works)
        if (peek_token(token_pointer)->type == TOKEN_DOT) {
          // Add 3 dots node aka infinite more parameters...
          break;
        }
        expect_token(TOKEN_COMMA, token_pointer);
      }
    }

    ast->type = NODE_FUNCTION_DECLARATION;
    ast = parse_scope(context, scope + 1, token_pointer);
    break;
  default:
    // Error
    return ast;
  }

  return ast;
}

int compare_string_vectors(const void *a, const void *b, void* _udata) {
  const char_vector string1 = (char_vector)a;
  const char_vector string2 = (char_vector)b;
  return strncmp(string1, string2, vector_size((vector*)&string1));
  vec_size_t i = 0;
  int difference_score = 0;
  while(true) {
    // If 'i' is bigger than either vector, stop.
    if (i >= vector_size((vector*)&string1)){
      break;
    }
    if (i >= vector_size((vector*)&string2)){
      break;
    }
    difference_score += abs(vector_get(&string2, i) - vector_get(&string1, i));
    i++;
  }
  return difference_score;
}


uint64_t hash_string_vector(const void *data, uint64_t seed0, uint64_t seed1) {
  const char_vector string = (char_vector)data;
  const size_t char_vector_size = vector_size((vector*)&string);
  return hashmap_sip(string, sizeof(char) * char_vector_size, seed0, seed1);
}

bool is_in_any_of_context_hashmaps(scope_context *context, char_vector name) {
  hashmap_vector type_hashmaps = context->type_hashmaps;
  // Loop through every type hashmap and check whether the name is in any of them.
  // Should make this a generic function but who cares.
  for(vec_size_t i = 0; i < vector_size((vector*)&type_hashmaps); i++) {
    if(hashmap_get(type_hashmaps[i], name) != NULL) {
      return true;
    }
  }
  return false;
}

// Parses a scope (block of tokens between braces) and turns it into an abstract syntax tree.
node *parse_scope(scope_context *context, int scope, token **token_pointer){
  struct hashmap *type_hashmap = hashmap_new(sizeof(char_vector), 0, 0, 0, hash_string_vector, compare_string_vectors, NULL, NULL);
  // If the vector has a previous hashmap already allocated, free it
  if(vector_has((vector*)&context->type_hashmaps, scope)) {
    hashmap_free(vector_get(&context->type_hashmaps, scope));
  }
  vector_add(&context->type_hashmaps, type_hashmap);

  token *current_token = peek_token(token_pointer);
  printf("%ld\n", hash_string_vector(current_token->value, 0, 0));
  printf(hashmap_get(context->type_hashmaps[0], current_token->value) ? "Ye it is valid get\n" : "nah it's null\n");
  node *ast = create_node(NODE_BLOCK);
  // Keep parsing individual statements until end of scope
  while(current_token->type != TOKEN_END) {
    current_token = peek_token(token_pointer);
    switch (current_token->type) {
    default:
      printf("Error in parse block, unknown type '%s'\n",
             token_type_to_string(current_token->type));
      exit(1);
      break;

    case TOKEN_TYPEDEF:
      hashmap_set(type_hashmap, current_token->value);
      break;

    case TOKEN_NAME:
      // If name is in 'type' hashmap, means parse it like a type
      // Ex: int number = 0;
      if (is_in_any_of_context_hashmaps(context, current_token->value)) {
        ast = parse_type(token_pointer, context, scope);
      } else {
        ast = parse_variable_assignment(token_pointer);
      }
      break;
    case TOKEN_END:
      // Add ending notifier
      expect_token(TOKEN_END, token_pointer);
      vector_add(&ast->block.nodes, *create_node(NODE_END));
      break;
    }
  }

  return ast;
}

// Questions for code style:
//
// What to return when parsing error? Do I exit?
// In this specific situation, just output NODE_NONE or make NODE_ERROR.
//
// Yk how enums have either super specific names (TOKEN_LEFT_PARENTHESES) or
// just broad terms (TOKEN_PUNCTUATION)? Which is better? Broad terms gonna be
// easier to sort, but harder to implement, more if statements. Super specific
// names though, are just a giant set of boilerplate waiting to happen.
//
// Also, should preprocessor go before or after lexer? In most C compilers, it's
// before. But that limits smart-ness and I can't think of another reason. But
// there is one.
//
// Switch or if? Probably switch.
//
// Man honestly I should commit this to git. But also this is an ugly,
// horrendous, unfinished slop of code that I don't want anyone to see. It'll
// clean up eventually, after I fix all of th errors and it works (in concept)
//
// When I'm working with a token or a node, what do I call it? I've started
// calling it current_token, or token, or ast(abstract syntax tree), or node,
// but I want to have a standard.
//

void print_ast(node *ast, int indent_level) {
  // Generate indents
  char *tabs = malloc(indent_level * sizeof(char));
  for (int i = 0; i < indent_level; i++) {
    tabs[i] = '\t';
  }

  // Print out this node's type
  printf("%s%s\n", tabs, node_type_to_string(ast->type));

  // Last: Print children
  // If the node's type is a block, print it's children.
  if (ast->type != NODE_BLOCK) {
    return;
  }
  for (int i = 0; i < (int)vector_size((vector*)&ast->block.nodes); i++) {
    print_ast(vector_get(&ast->block.nodes, i), indent_level + 1);
  }

  // TODO: print out the value of this node
}

// Takes in tokens, outputs an Abstract Syntax Tree (AST)
node *parser(token *tokens) {
  // Pointer to pointer so we can store state of which token we're on
  token *end_token = (token*)vector_last(&tokens);
  assert(end_token->type == TOKEN_END);

  token **token_pointer = &tokens;
  // Requires: access tokens, context info (types, ...), depth/indentation/scope
  // typedef int myint;
  // myint i = 0;
  scope_context context = {
    .type_hashmaps = vector_create(),
    .var_hashmaps = vector_create(),
  };
  struct hashmap *type_hashmap = hashmap_new(sizeof(char_vector), 0, 0, 0, hash_string_vector, compare_string_vectors, NULL, NULL);
  vector_add(&context.type_hashmaps, type_hashmap);
  char_vector int_type = vector_create();
  vector_add(&int_type, 'i');
  vector_add(&int_type, 'n');
  vector_add(&int_type, 't');
  vector_add(&int_type, '\0');
  printf("%ld\n", hash_string_vector(int_type, 0, 0));

  // Just a reminder, 'hashmap_set' takes in hashmap and object.
  // Object has the key (usually object.name), and itself as the value.
  // That is why it is only the 'key' instead of 'key:value'
  hashmap_set(type_hashmap, int_type);

  // Parse scope (with a scope number of one, we declared int for 0'th layer)
  node *ast = parse_scope(&context, 1, token_pointer);

  print_ast(ast, 0);

  return ast;
}
