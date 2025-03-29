#include "parser.h"
#include "c-tests/test.h"
#include "c-vector/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *node_type_strings[] = {ITERATE_NODES_AND(GENERATE_STRING)};
const char *operator_type_strings[] = {ITERATE_OPERATORS_AND(GENERATE_STRING)};

const char *node_type_to_string(node_type type) {
  return enum_to_string(type, node_type_strings);
}
const char *operator_type_to_string(operator_type type) {
  return enum_to_string(type, operator_type_strings);
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

token *expect_token(token_type type, token **token_pointer) {
  token *current_token = pop_token(token_pointer);
  if (current_token->type != type) {
    printf("Token given: %s \n Token expected: %s \n",
           token_type_to_string(current_token->type),
           token_type_to_string(type));
  }
  return current_token;
}

node *create_node(node_type type) {
  node *current_node = malloc(sizeof(node));
  current_node->type = type;
  return current_node;
}

bool is_at_end(token **token_pointer) {
  return peek_token(token_pointer)->type == TOKEN_END;
}

int how_many_in_a_row(token_type type, token **token_pointer) {
  int number = 0;
  while (true) {
    if (peek_token(token_pointer)->type != type) {
      break;
    }
    advance_token(token_pointer);
    number++;
  }
  return number;
}

int compare_string_vectors(const void *a, const void *b, void *_udata) {
  const char_vector string1 = (char_vector)a;
  const char_vector string2 = (char_vector)b;
  return strncmp(string1, string2, vector_size((vector *)&string1));
}

uint64_t hash_string_vector(const void *data, uint64_t seed0, uint64_t seed1) {
  const char_vector string = (char_vector)data;
  const size_t char_vector_size = vector_size((vector *)&string);
  return hashmap_sip(string, sizeof(char) * char_vector_size, seed0, seed1);
}

void create_or_clear_context(scope_context *context, int depth) {
  if (vector_has((vector *)&context->type_hashmaps, depth)) {
    struct hashmap *type_hashmap = vector_get(&context->type_hashmaps, depth);
    hashmap_clear(type_hashmap, true);
  } else {
    struct hashmap *type_hashmap = hashmap_new(sizeof(char_vector), 0, 0, 0, hash_string_vector, compare_string_vectors, NULL, NULL);
    vector_insert(&context->type_hashmaps, depth, type_hashmap);
  }
}

void add_type_to_context(scope_context *context, char_vector type_name, int depth) {
  // Just a reminder, 'hashmap_set' takes in hashmap and object.
  // Object has the key (usually object.name), and itself as the value.
  // That is why it is only the 'key' instead of 'key:value'
  hashmap_set(context->type_hashmaps[depth], type_name);
}

bool is_type(scope_context *context, char_vector name) {
  hashmap_vector type_hashmaps = context->type_hashmaps;
  for (vec_size_t i = 0; i < vector_size((vector *)&type_hashmaps); i++) {
    if (hashmap_get(type_hashmaps[i], name) != NULL) {
      return true;
    } else {
      assert("Dang yeah word isn't in context/type hashmaps\n" == NULL);
    }
  }
  return false;
}

// Prototype for later lines
node *parse_block(scope_context *context, int scope, token **token_pointer);
// Prototype because it complains about multiple function definitions (It's used
// before it's defined).
node *parse_precedence(precedence precedence, token **token_pointer);

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

node *parse_number(token **token_pointer) {
  node *number_node = create_node(NODE_NUMBER_LITERAL);
  // Get value from that token that sent us here
  int value = atoi(pop_token(token_pointer)->value);
  number_node->number_literal.value = value;

  return number_node;
}

node *parse_grouping(token **token_pointer) {
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  node *current_node = parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  return current_node;
}

node *parse_binary(node *last_expression, token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  node *next_expression =
      parse_precedence(get_precedence(operator_token->type) + 1, token_pointer);

  operator_type operator;
  switch (operator_token->type) {
  case TOKEN_PLUS:
    operator= OPERATOR_ADD;
    break;
  case TOKEN_MINUS:
    operator= OPERATOR_SUBTRACT;
    break;
  case TOKEN_STAR:
    operator= OPERATOR_MULTIPLY;
    break;
  case TOKEN_SLASH:
    operator= OPERATOR_DIVIDE;
    break;
  default:
    printf("Error in parse binary");
    break;
  }

  node *ast = create_node(NODE_EQUATION);
  ast->equation.operator= operator;
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
    operator= OPERATOR_NEGATE;
    break;
  case TOKEN_NOT:
    operator= OPERATOR_NOT;
    break;
  default:
    printf("Error in parse unary");
    break;
  }

  node *current_node = create_node(NODE_EQUATION);
  current_node->equation.operator= operator;
  current_node->equation.left = expression;
  current_node->equation.right = NULL;

  return current_node;
}

node *parse_function_call(node *function_expression, token **token_pointer) {
  // Function expression is the stuff before parentheses
  // `variable.function_pointer`
  // (
  // expression, expression
  // )
  // we don't care about what's after this call
  //
  node *current_node = create_node(NODE_CALL_FUNCTION);
  current_node->function_call.function_expression = function_expression;
  current_node->function_call.inputs = vector_create();

  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  // Loop till end or TOKEN_RIGHT_PARENTHESES
  // Example: func(1 + 5 + a, "string");
  while (!is_at_end(token_pointer) &&
         peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
    vector_add(&current_node->function_call.inputs,
               parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer));
    // If we parse expression and token on right is anything but comma or
    // parentheses, syntax error
    if (peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
      expect_token(TOKEN_COMMA, token_pointer);
    }
  }
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);

  return current_node;
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

  node *current_node = NULL;
  node *last_expression = NULL;
  node *current_expression = NULL;

  // Keep parsing equal or higher precedence values
  // Example: -12 - -6 * (2 + 3);
  while (precedence <= get_precedence(current_token->type)) {
    switch (current_token->type) {
    case TOKEN_COMMA:
    case TOKEN_SEMI_COLON:
      // Error here, semi colon not supposed to be in the middle of expression.
      // It doesn't make sense, but in "get_precedence", semi colon means "just
      // return, don't loop a last time"
      return current_node;

    // Imagine a scenario like this: (get_object()->number++) + 5
    case TOKEN_LEFT_PARENTHESES:
      current_expression = parse_grouping(token_pointer);
      break;

    case TOKEN_NUMBER:
      current_expression = parse_number(token_pointer);
      break;

    case TOKEN_DOT:
      // Get rid of '.'
      advance_token(token_pointer);
      current_expression = create_node(NODE_STRUCT_MEMBER_GET);
      // Last expression is variable (variable. <-we're here)
      current_expression->struct_member_get.from = last_expression;
      current_expression->struct_member_get.name =
          pop_token(token_pointer)->value;
      // Now, we've parsed to here: (variable.member <-we're here)
      if (peek_token(token_pointer)->type == TOKEN_LEFT_PARENTHESES) {
        current_expression =
            parse_function_call(current_expression, token_pointer);
      }
      break;

    case TOKEN_NAME:
      current_expression = create_node(NODE_VARIABLE);
      current_expression->variable.name = pop_token(token_pointer)->value;
      // Function call by itself
      if (peek_token(token_pointer)->type == TOKEN_LEFT_PARENTHESES) {
        current_expression =
            parse_function_call(current_expression, token_pointer);
      }
      // Honestly, just ignore if it's a type or not. Symantic analysis will do
      // that for us "name" and nothing else == variable "name.name" == struct
      // member get, should be recursive for "name.name.name.name..."
      // "name->name" == struct dereference and member get
      // "name(name name, name name, ...)" == function call
      //
      // Use "variable" and "structure" nodes
      // We also gotta check a dereference (*) before this token
      // Use dereference and reference for the operators
      break;

    case TOKEN_EQUALS_EQUALS:
    case TOKEN_NOT_EQUALS:
    case TOKEN_LESS_THAN:
    case TOKEN_LESS_THAN_EQUALS:
    case TOKEN_GREATER_THAN:
    case TOKEN_GREATER_THAN_EQUALS:
    case TOKEN_AMPERSAND:
    case TOKEN_AND:
    case TOKEN_PIPE:
    case TOKEN_OR:
    case TOKEN_CARET:
    case TOKEN_PLUS:
    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MINUS_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PERCENT:
      current_expression = parse_binary(last_expression, token_pointer);
      break;

    default:
      // TODO: Better error handling
      printf("Error when parsing precedence: Unknown token %s",
             token_type_to_string(current_token->type));
      return current_node;
    }
    current_token = peek_token(token_pointer);
    last_expression = current_expression;
    current_node = current_expression;
  }

  return current_node;
}

node *parse_expression(token **token_pointer) {
  node *current_node = parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  return current_node;
}

// The "current_node" parameter is passed in with "type", "name", and "pointer" amount already set
node *parse_function(node *current_node, token **token_pointer) {
  current_node->function.parameters = vector_create();
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);

  // Loop till end or TOKEN_RIGHT_PARENTHESES
  while (!is_at_end(token_pointer) && peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
    type_info type = {
        .name = expect_token(TOKEN_NAME, token_pointer)->value,
        .pointer_amount = how_many_in_a_row(TOKEN_STAR, token_pointer),
    };
    parameter current_parameter = {
        .type = type,
        .name = expect_token(TOKEN_NAME, token_pointer)->value,
    };
    vector_add(&current_node->function.parameters, current_parameter);
    // If we parse expression and token on right is anything but comma or parentheses, syntax error
    if (peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
      expect_token(TOKEN_COMMA, token_pointer);
    }
  }
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  return current_node;
}

// parse_variable_assignment and parse_variable_declaration seem to be very similar
// I wish I could somehow merge them
// 
// Parse something like "a = 12 + (9 * 20);"
node *parse_variable_assignment(token **token_pointer) {
  node *current_node = create_node(NODE_VARIABLE_DECLARATION);
  // TODO: Figure out whether to make new node or roll with 'no type'
  // current_node->variable.type = NULL;
  current_node->variable_declaration.name = expect_token(TOKEN_NAME, token_pointer)->value;
  expect_token(TOKEN_EQUALS, token_pointer);
  current_node->variable_declaration.value = parse_expression(token_pointer);
  return current_node;
}

node *parse_variable_declaration(node *current_node, token **token_pointer) {
  expect_token(TOKEN_EQUALS, token_pointer);
  current_node->type = NODE_VARIABLE_DECLARATION;
  current_node->variable_declaration.value = parse_expression(token_pointer);
  return current_node;
}

node *parse_variable_prototype(node *current_node, token **token_pointer) {
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  current_node->type = NODE_VARIABLE_DECLARATION;
  current_node->variable_declaration.value = NULL;
  return current_node;
}

// Should be a stack struct such as "parameter" so we only give the info that is required
// Aka, type, pointer amount, and name
// This is a measure that should be taken as this mechanism is used everywhere
node *parse_variable_attributes(token **token_pointer) {
  // Get the type, how many references, then the name of function/variable
  type_info type = {
    .name = expect_token(TOKEN_NAME, token_pointer)->value,
    .pointer_amount = how_many_in_a_row(TOKEN_STAR, token_pointer),
  };
  char_vector name = expect_token(TOKEN_NAME, token_pointer)->value;
  
  assert(vector_size((vector*)&type.name) > 0);
  assert(vector_size((vector*)&name) > 0);

  node *current_node = create_node(NODE_NONE);
  // Set the type_info and name for variables AND functions AND parameters
  current_node->variable_declaration.type = type;
  current_node->variable_declaration.name = name;

  return current_node;
}

// Parse a variable or function declaration.
node *parse_type(scope_context *context, int depth, token **token_pointer) {
  node *current_node = parse_variable_attributes(token_pointer);
  switch (peek_token(token_pointer)->type) {
  case TOKEN_EQUALS:
    current_node = parse_variable_declaration(current_node, token_pointer);
    break;
  case TOKEN_SEMI_COLON:
    current_node = parse_variable_prototype(current_node, token_pointer);
    break;
  case TOKEN_LEFT_PARENTHESES:
    current_node = parse_function(current_node, token_pointer);
    // Why not parse the body in the "parse function" function???? Bc requires too many parameters
    current_node->function.body = parse_block(context, depth + 1, token_pointer);
    break;
  default:
    // Error
    printf("Error in parse_type\n");
    return current_node;
  }

  assert(current_node->type != NODE_NONE);
  return current_node;
}

node *parse_loop(token **token_pointer) {
  int i = 0;
  while ((bool)(i + 1 < 5)) {
  }
  // Uhhh, Idk what to do about parsing types in like for loops
  // Condition
  // Block of code
}

// Parses a block of tokens between braces, and turns it into an abstract syntax tree.
node *parse_block(scope_context *context, int depth, token **token_pointer) {
  assert(context != NULL);
  assert(vector_size((vector *)&context->type_hashmaps) > 0);
  assert(depth > 0 && depth < 100);
  assert(token_pointer != NULL);

  create_or_clear_context(context, depth);

  node *ast = create_node(NODE_BLOCK);
  ast->block.nodes = vector_create();
  node *current_node = NULL;
  token *current_token = peek_token(token_pointer);

  // Keep parsing individual statements until end of scope
  while (current_token->type != TOKEN_END) {
    switch (current_token->type) {
    default:
      printf("Error in parse block, unknown type '%s'\n", token_type_to_string(current_token->type));
      exit(1);
      break;

    case TOKEN_TYPEDEF:
      add_type_to_context(context, current_token->value, depth);
      break;

    case TOKEN_NAME:
      // Example: int j = k + 5; versus j = 2;
      if (is_type(context, current_token->value)) {
        current_node = parse_type(context, depth, token_pointer);
      } else {
        current_node = parse_variable_assignment(token_pointer);
      }
      break;
    case TOKEN_END:
      // Add ending notifier
      expect_token(TOKEN_END, token_pointer);
      current_node = create_node(NODE_END);
      break;
    }
    assert(current_node != NULL);
    vector_add(&ast->block.nodes, current_node);
    current_token = peek_token(token_pointer);
  }

  return ast;
}

void print_block(node *ast, int indent_level) {
  // Generate indents
  for (int i = 0; i < indent_level; i++) {
    putchar('\t');
  }

  printf("%s\n", node_type_to_string(ast->type));

  node *next_ast = NULL;
  switch (ast->type) {
  default:
    break;
  case NODE_BLOCK:
    for (int i = 0; i < (int)vector_size((vector *)&ast->block.nodes); i++) {
      print_block(vector_get(&ast->block.nodes, i), indent_level + 1);
    }
    break;
  case NODE_VARIABLE_DECLARATION:
    for (int i = 0; i < indent_level; i++) {
      putchar('\t');
    }
    vector_print_string(&ast->variable_declaration.name);
    printf("\n");
    next_ast = ast->variable_declaration.value;
    if (next_ast != NULL) {
      print_block(next_ast, indent_level + 1);
    }
    break;
  case NODE_EQUATION:
    for (int i = 0; i < indent_level; i++) {
      putchar('\t');
    }
    printf(": %s\n", operator_type_to_string(ast->equation.operator));
    next_ast = ast->equation.left;
    if (next_ast != NULL) {
      print_block(next_ast, indent_level + 1);
    }
    next_ast = ast->equation.right;
    if (next_ast != NULL) {
      print_block(next_ast, indent_level + 1);
    }
    break;
  }
}


// Takes in tokens, outputs an Abstract Syntax Tree (AST)
node *parser(token *tokens) {
  // Check if token at end is end token.
  assert(((token *)vector_last(&tokens))->type == TOKEN_END);

  // Pointer to pointer so we can store state of which token we're on
  token **token_pointer = &tokens;

  // Requires: access tokens, context info (types, ...), depth/indentation/scope
  // typedef int myint;
  // myint i = 0;
  scope_context context = {
      .type_hashmaps = vector_create(),
      .var_hashmaps = vector_create(),
  };

  create_or_clear_context(&context, 0);
  add_type_to_context(&context, _vector_from("int", sizeof(char), 4), 0);

  // Parse scope (with a scope number of one, we declared int for 0'th layer)
  node *ast = parse_block(&context, 1, token_pointer);

  print_block(ast, 0);

  return ast;
}
