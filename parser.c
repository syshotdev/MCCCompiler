#include "parser.h"
#include "c-tests/test.h"
#include "c-vector/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNREASONABLE_TYPE_NUMBER 1000
#define UNREASONABLE_CONTEXT_NUMBER 1000


const char *node_type_strings[] = { ITERATE_NODES_AND(GENERATE_STRING) };
const char *operator_type_strings[] = { ITERATE_OPERATORS_AND(GENERATE_STRING) };

const char *node_type_to_string(node_type type) {
  return enum_to_string(type, node_type_strings);
}
const char *operator_type_to_string(operator_type type) {
  return enum_to_string(type, operator_type_strings);
}

// Base functions

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
bool is_at_end(token **token_pointer) {
  return peek_token(token_pointer)->type == TOKEN_END;
}

node *create_node(node_type type) {
  node *current_node = malloc(sizeof(node));
  current_node->type = type;
  assert(current_node != NULL);
  return current_node;
}

// Types

int compare_typedef_entries(const void *a, const void *b, void *udata) {
  (void)udata;
  const char_vector name1 = ((typedef_entry *)a)->name;
  const char_vector name2 = ((typedef_entry *)b)->name;
  if (vector_size((vector *)&name1) != vector_size((vector *)&name2)){
    return 1;
  }
  return strncmp(name1, name2, vector_size((vector *)&name1));
}
uint64_t hash_typedef_entry(const void *data, uint64_t seed0, uint64_t seed1) {
  const char_vector name = ((typedef_entry *)data)->name;
  return hashmap_sip(name, vector_size((vector *)&name), seed0, seed1);
}
void create_or_clear_context(scope_context context) {
  if (vector_has((vector *)&context.typedef_hashmaps, context.depth)) {
    struct hashmap *type_hashmap = *(struct hashmap**)vector_get(&context.typedef_hashmaps, context.depth);
    hashmap_clear(type_hashmap, true);
  } else {
    struct hashmap *type_hashmap = hashmap_new(sizeof(typedef_entry), 1, 0, 0, hash_typedef_entry, compare_typedef_entries, NULL, NULL);
    vector_insert(&context.typedef_hashmaps, context.depth, type_hashmap);
    assert(*(struct hashmap**)vector_get(&context.typedef_hashmaps, context.depth) == type_hashmap);
  }
}

void add_type_to_context(typedef_entry object, scope_context context) {
  // Just a reminder, 'hashmap_set' takes in hashmap and object.
  // Object has the key (usually object.name), and itself as the value.
  // That is why it is only the 'key' instead of 'key:value'
  hashmap_set(context.typedef_hashmaps[context.depth], &object);
  typedef_entry *debug_object = (typedef_entry *)hashmap_get(context.typedef_hashmaps[context.depth], &object);
  assert(debug_object != NULL);
  assert(vector_compare((vector *)&debug_object->name, (vector *)&object.name) == 0);
}

bool is_type(char_vector name, scope_context context) {
  hashmap_vector typedef_hashmaps = context.typedef_hashmaps;
  assert(typedef_hashmaps[0] == context.typedef_hashmaps[0]);
  for (vec_size_t i = 0; i <= (vec_size_t)context.depth; i++) {
    if (hashmap_get(typedef_hashmaps[i], &(typedef_entry){ .name=name }) != NULL) {
      return true;
    } else {
      printf("Info: '%.*s' isn't in context/type hashmaps\n", (int)vector_size((vector*)&name), name);
    }
  }
  return false;
}

// Parse and collect struct members
node_vector collect_members(scope_context context, token **token_pointer) {
  token_type right_break_token = TOKEN_RIGHT_BRACE;
  node_vector members = vector_create(); 
  while (!is_at_end(token_pointer) &&
      peek_token(token_pointer)->type != right_break_token) {
    node *member = parse_type_expression(context, token_pointer);
    vector_add(&members, member);
    assert((*(node **)vector_last(&members))->type == member->type);

    expect_token(TOKEN_SEMI_COLON, token_pointer);
  }
  return members;
}

// Parse and collect typed nodes separated by commas
// Ex: (int i, struct Point {...}, ...)
node_vector collect_parameters(scope_context context, token **token_pointer) {
  token_type right_break_token = TOKEN_RIGHT_PARENTHESES;
  node_vector parameters = vector_create(); 
  while (!is_at_end(token_pointer) &&
      peek_token(token_pointer)->type != right_break_token) {
    node *parameter = parse_type_expression(context, token_pointer);
    vector_add(&parameters, parameter);
    assert((*(node **)vector_last(&parameters))->type == parameter->type);

    token *current_token = peek_token(token_pointer);
    if (current_token->type == TOKEN_COMMA) {
      expect_token(TOKEN_COMMA, token_pointer);
      continue;
    } else if (current_token->type == right_break_token) {
      continue;
    } else {
      error("Expected ',' or '%s', got '%s'",
            token_type_to_string(right_break_token),
            token_type_to_string(current_token->type));
    }
  }
  return parameters;
}

// Parses pointers for types if they are there
node *parse_pointers(node *type_node, token **token_pointer) {
  // For each star, create a type node, point to type, then set type to the pointer to do it again
  while (peek_token(token_pointer)->type == TOKEN_STAR) {
    expect_token(TOKEN_STAR, token_pointer);
    node *pointer = create_node(NODE_POINTER);
    pointer->pointer.to = type_node;
    type_node = pointer;
  }
  return type_node;
}

// Just parses "int" into a type
node *parse_base_type(scope_context context, token **token_pointer) {
  token *type_token = expect_token(TOKEN_NAME, token_pointer);
  if (is_type(type_token->value, context)) {
    node *type_node = create_node(NODE_TYPE);
    type_node->base_type.name = type_token->value;
    return type_node;
  } else {
    error("Supposed '%s' is not a type.", type_token->value);
  }
}

// Parse structs
node *parse_structure_type(scope_context context, token **token_pointer) {
  expect_token(TOKEN_STRUCT, token_pointer);
  node *struct_node = create_node(NODE_TYPE);
  token *current_token = peek_token(token_pointer);

  if (current_token->type == TOKEN_NAME) {
    struct_node->structure.name = expect_token(TOKEN_NAME, token_pointer)->value;
  } else {
    struct_node->structure.name = NULL;
  }

  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  struct_node->structure.members = collect_members(context, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  return struct_node;
}

// Parse a type like an `int**` or a `struct`, and nothing more
node *parse_type(scope_context context, token **token_pointer) {
  node *type_node = NULL;
  token_type type = peek_token(token_pointer)->type;

  assert(type < UNREASONABLE_TYPE_NUMBER);

  switch (type) {
  case TOKEN_STRUCT:
    type_node = parse_structure_type(context, token_pointer);
    break;
  case TOKEN_NAME:
    type_node = parse_base_type(context, token_pointer);
    break;
  default:
    error("Unknown type '%s', perhaps I haven't implemented it yet?", 
        token_type_to_string(type));
  }

  type_node = parse_pointers(type_node, token_pointer);

  return type_node;
}

// Parses a type and the expression after it.
node *parse_type_expression(scope_context context, token **token_pointer) {
  node *type_expression = create_node(NODE_NONE);
  // The type variable is located in the same place for functions and variable_declarations
  type_expression->variable_declaration.type = parse_type(context, token_pointer);
  // TODO: Make struct specific path where it can have no name, and it's just the type definition.
  // Structs can have no name
  if (peek_token(token_pointer)->type == TOKEN_NAME) {
    type_expression->variable_declaration.name = expect_token(TOKEN_NAME, token_pointer)->value;
  } else {
    type_expression->variable_declaration.name = NULL;
  }

  assert(type_expression->variable_declaration.type != NULL);

  switch (peek_token(token_pointer)->type) {
  case TOKEN_EQUALS:
    expect_token(TOKEN_EQUALS, token_pointer);
    type_expression->type = NODE_VARIABLE_DECLARATION;
    type_expression->variable_declaration.value = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
    break;
  case TOKEN_COMMA:
    type_expression->type = NODE_VARIABLE_DECLARATION;
    type_expression->variable_declaration.value = NULL;
    break;
  case TOKEN_SEMI_COLON:
    type_expression->type = NODE_VARIABLE_DECLARATION;
    type_expression->variable_declaration.value = NULL;
    break;
  case TOKEN_LEFT_PARENTHESES:
    type_expression = parse_function(type_expression, context, token_pointer);
    break;
  default:
    error("Unknown token: '%s'", token_type_to_string(peek_token(token_pointer)->type));
    return type_expression;
  }

  assert(type_expression != NULL);
  return type_expression;
}

// Parsing helpers

precedence get_precedence(token_type type) {
  precedence precedence = PRECEDENCE_ASSIGNMENT;
  switch (type) {
  default:
    precedence = PRECEDENCE_NONE;
    break;
  // If there's a while(condition), the right parentheses should not be used in the loop
  case TOKEN_RIGHT_PARENTHESES:
    precedence = PRECEDENCE_NONE;
    break;
  case TOKEN_EQUALS:
  case TOKEN_PLUS_EQUALS:
  case TOKEN_MINUS_EQUALS:
  case TOKEN_PLUS_PLUS:
  case TOKEN_MINUS_MINUS:
    precedence = PRECEDENCE_ASSIGNMENT;
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

operator_type get_binary_type(token_type type) {
  operator_type operator;
  switch (type) {
  // All of these technically reassign a variable, but categories like ++ and -=
  // must have their own edge cases to be parsed correctly
  case TOKEN_EQUALS:
  case TOKEN_PLUS_EQUALS:
  case TOKEN_MINUS_EQUALS:
  case TOKEN_PLUS_PLUS:
  case TOKEN_MINUS_MINUS:
    operator = OPERATOR_ASSIGN;
    break;
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
  case TOKEN_AMPERSAND:
    operator = OPERATOR_AND;
    break;
  case TOKEN_PIPE:
    operator = OPERATOR_OR;
    break;
  case TOKEN_CARET:
    operator = OPERATOR_XOR;
    break;
  case TOKEN_NOT:
    operator = OPERATOR_NOT;
    break;
  case TOKEN_NOT_EQUALS:
    operator = OPERATOR_NOT_EQUALS;
    break;
  case TOKEN_EQUALS_EQUALS:
    operator = OPERATOR_EQUALS_EQUALS;
    break;
  case TOKEN_LESS_THAN:
    operator = OPERATOR_LESS_THAN;
    break;
  case TOKEN_LESS_THAN_EQUALS:
    operator = OPERATOR_LESS_THAN_EQUALS;
    break;
  case TOKEN_GREATER_THAN:
    operator = OPERATOR_GREATER_THAN;
    break;
  case TOKEN_GREATER_THAN_EQUALS:
    operator = OPERATOR_GREATER_THAN_EQUALS;
    break;
  case TOKEN_AND:
    operator = OPERATOR_BOOLEAN_AND;
    break;
  case TOKEN_OR:
    operator = OPERATOR_BOOLEAN_OR;
    break;
  default:
    error("Unknown operator '%s'\n", token_type_to_string(type));
    operator = OPERATOR_ADD;
    break;
  }
  return operator;
}

bool should_parse_unary(token_type type) {
  switch (type) {
  default:
    return false;
  case TOKEN_MINUS:
  case TOKEN_PLUS:
  case TOKEN_NOT:
    return true;
  }
}

node *create_equation_equals_equation_and_next(node *last_expression, node *next_expression, operator_type operator) {
  // i + <EXPRESSION>
  node *plus_expression = create_node(NODE_EQUATION); // Idk what to name this
  plus_expression->equation.operator = operator;
  plus_expression->equation.left = last_expression;
  plus_expression->equation.right = next_expression;

  // i = i + <EXPRESSION>
  node *current_expression = create_node(NODE_EQUATION);
  current_expression->equation.operator = OPERATOR_ASSIGN;
  current_expression->equation.left = last_expression;
  current_expression->equation.right = plus_expression;
  return current_expression;
}

node *create_struct_member_get(node *from_expression, token **token_pointer) {
  node *current_node = create_node(NODE_STRUCT_MEMBER_GET);
  current_node->struct_member_get.from = from_expression;
  // Get the name from the next token, that is the member we're trying to get
  current_node->struct_member_get.name = expect_token(TOKEN_NAME, token_pointer)->value;
  return current_node;
}

// Parsing (yay finally after 350 lines!!!)

node *parse_string(token **token_pointer) {
  node *string_node = create_node(NODE_STRING);
  string_node->string.value = expect_token(TOKEN_STRING, token_pointer)->value;
  return string_node;
}

node *parse_number(token **token_pointer) {
  node *number_node = create_node(NODE_NUMBER_LITERAL);
  int value = atoi(expect_token(TOKEN_NUMBER, token_pointer)->value);
  number_node->number_literal.value = value;

  return number_node;
}

// `i->foo.function()`
node *parse_variable_expression(token **token_pointer) {
  node *current_expression = create_node(NODE_VARIABLE);
  current_expression->variable.name = expect_token(TOKEN_NAME, token_pointer)->value;

  while(true) {
    token *operator_token = peek_token(token_pointer);
    switch (operator_token->type){
    default:
      return current_expression;
    case TOKEN_ARROW:
      expect_token(TOKEN_ARROW, token_pointer);
      current_expression = parse_struct_member_dereference_get(current_expression, token_pointer);
      break;
    case TOKEN_DOT:
      expect_token(TOKEN_DOT, token_pointer);
      current_expression = parse_struct_member_get(current_expression, token_pointer);
      break;
    case TOKEN_LEFT_PARENTHESES:
      expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
      current_expression = parse_function_call(current_expression, token_pointer);
      expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
      break;
    }
  }
}

// `variable.member` <- This last part
node *parse_struct_member_get(node *from_expression, token **token_pointer) {
  expect_token(TOKEN_DOT, token_pointer);
  return create_struct_member_get(from_expression, token_pointer);
}

node *parse_struct_member_dereference_get(node *from_expression, token **token_pointer) {
  node *current_expression = create_node(NODE_EQUATION);
  current_expression->equation.operator = OPERATOR_DEREFERENCE;
  current_expression->equation.left = create_struct_member_get(from_expression, token_pointer);
  current_expression->equation.right = NULL;
  return current_expression;
}

// Parse the actual function
node *parse_function(node *function_expression, scope_context context, token **token_pointer) {
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  function_expression->function.parameters = collect_parameters(context, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  function_expression->function.body = parse_block(context, token_pointer);
  return function_expression;
}

// variable.function_pointer(expression, expression)
// We don't care about what's after this call
node *parse_function_call(node *from_expression, token **token_pointer) {
  assert(from_expression != NULL);

  node *current_node = create_node(NODE_FUNCTION_CALL);
  current_node->function_call.function_expression = from_expression;
  current_node->function_call.inputs = vector_create();

  // Loop till end or TOKEN_RIGHT_PARENTHESES
  // Example: `func(1 + 5 + a, "string");`
  while (!is_at_end(token_pointer) && peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
    node *parameter_expression = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
    vector_add(&current_node->function_call.inputs, parameter_expression);
    assert(*(node **)vector_last(&current_node->function_call.inputs) == parameter_expression);
    // If we parse expression and token on right is anything but comma or parentheses, syntax error
    if (peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
      expect_token(TOKEN_COMMA, token_pointer);
    }
  }

  return current_node;
}

node *parse_grouping(token **token_pointer) {
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  node *expression = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  return expression;
}

node *parse_binary(node *last_expression, token **token_pointer) {
  token *operator_token = pop_token(token_pointer);

  operator_type operator = get_binary_type(operator_token->type);
  node *next_expression = parse_expression(get_precedence(operator_token->type) + 1, token_pointer);

  assert(last_expression != NULL);
  assert(next_expression != NULL);
  assert(operator_token->type < UNREASONABLE_TYPE_NUMBER);

  node *current_node = create_node(NODE_EQUATION);

  switch (operator_token->type) {
  case TOKEN_PLUS_EQUALS:
    current_node = create_equation_equals_equation_and_next(last_expression, next_expression, OPERATOR_ADD);
    break;
  case TOKEN_MINUS_EQUALS:
    current_node = create_equation_equals_equation_and_next(last_expression, next_expression, OPERATOR_SUBTRACT);
    break;
  default:
    current_node->equation.operator = operator;
    current_node->equation.left = last_expression;
    current_node->equation.right = next_expression;
    break;
  }

  return current_node;
}

node *parse_unary(token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  operator_type operator = OPERATOR_NEGATE;
  assert(operator_token->type < UNREASONABLE_TYPE_NUMBER);
  switch (operator_token->type) {
  case TOKEN_PLUS:
    // +NUMBER does nothing
    return parse_expression(PRECEDENCE_UNARY, token_pointer);
  case TOKEN_MINUS:
    operator = OPERATOR_NEGATE;
    break;
  case TOKEN_NOT:
    operator = OPERATOR_NOT;
    break;
  case TOKEN_STAR:
    operator = OPERATOR_DEREFERENCE;
    break;
  default:
    error("Unknown operator '%s'\n", token_type_to_string(operator_token->type));
    return parse_expression(PRECEDENCE_UNARY, token_pointer);
  }

  node *current_expression = create_node(NODE_EQUATION);
  current_expression->equation.operator = operator;
  current_expression->equation.left = parse_expression(PRECEDENCE_UNARY, token_pointer);
  current_expression->equation.right = NULL;
  assert(current_expression->equation.left != NULL);
  assert(current_expression->equation.right == NULL);
  return current_expression;
}

node *parse_postfix(node *last_expression, token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  operator_type operator = OPERATOR_SUBTRACT;
  switch (operator_token->type) {
  default:
    error("Unknown operator '%s'\n", token_type_to_string(operator_token->type));
    break;
  case TOKEN_PLUS_PLUS:
    operator = OPERATOR_ADD;
    break;
  case TOKEN_MINUS_MINUS:
    operator = OPERATOR_SUBTRACT;
    break;
  }
  // `1`
  node *one = create_node(NODE_NUMBER_LITERAL);
  one->number_literal.value = 1;

  // `i + 1`
  node *plus_expression = create_node(NODE_EQUATION); // Idk what to name this
  plus_expression->equation.operator = operator;
  plus_expression->equation.left = last_expression;
  plus_expression->equation.right = one;

  // `i = i + 1`
  node *current_expression = create_node(NODE_EQUATION);
  current_expression->equation.operator = OPERATOR_ASSIGN;
  current_expression->equation.left = last_expression;
  current_expression->equation.right = plus_expression;
  return current_expression;
}


node *switch_expression(node *last_expression, node *current_expression, token_type type, token **token_pointer) {
  switch (type) {
  case TOKEN_COMMA:
  case TOKEN_SEMI_COLON:
  case TOKEN_RIGHT_PARENTHESES:
    // If this path is taken, it means a function overstepped it's boundaries and met
    // a semicolon, right parentheses, or a comma. For now, an expression is numbers, variables, and function
    // calls, and commas do not mean you can put several expressions in the same place.
    error("This path is not supposed to be reached.");
    break;

  case TOKEN_NAME:
    // TODO: Eventually, put "int" type expression parsing in here.
    // We can check the semantics later, functions in functions are not a problem.
    current_expression = parse_variable_expression(token_pointer);
    break;

  case TOKEN_STRING:
    current_expression = parse_string(token_pointer);
    break;

  case TOKEN_NUMBER:
    current_expression = parse_number(token_pointer);
    break;

  case TOKEN_LEFT_PARENTHESES:
    current_expression = parse_grouping(token_pointer);
    break;

  case TOKEN_PLUS_PLUS:
  case TOKEN_MINUS_MINUS:
    current_expression = parse_postfix(last_expression, token_pointer);
    break;

  case TOKEN_EQUALS:
  case TOKEN_PLUS_EQUALS:
  case TOKEN_MINUS_EQUALS:
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
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_PERCENT:
    current_expression = parse_binary(last_expression, token_pointer);
    break;

  default:
    // TODO: Better error handling
    printf("Error when parsing expression: Unknown token '%s'\n", token_type_to_string(type));
    return current_expression;
  }
  return current_expression;
}

// Used to be "parse_precedence" but I renamed it for clarity.
// Parses equal or higher precedense values (to ensure correct evaluation order)
// Example of what it will parse: variable.add_function(2, 3) * -12 + -i++
node *parse_expression(precedence precedence, token **token_pointer) {
  token *current_token = peek_token(token_pointer);

  if (should_parse_unary(current_token->type)) {
    return parse_unary(token_pointer);
  }

  node *current_expression = NULL;
  node *last_expression = NULL;

  while (precedence <= get_precedence(current_token->type)) {
    current_expression = switch_expression(current_expression, last_expression, current_token->type, token_pointer);
    last_expression = current_expression;
    current_token = peek_token(token_pointer);
  }
  
  return current_expression;
}

node *parse_do_while(scope_context context, token **token_pointer) {
  node *current_node = create_node(NODE_DO_WHILE);
  expect_token(TOKEN_DO, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->do_while_loop.body = parse_block(context, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  expect_token(TOKEN_WHILE, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  current_node->do_while_loop.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  assert(current_node != NULL);
  return current_node;
}

node *parse_while(scope_context context, token **token_pointer) {
  node *current_node = create_node(NODE_WHILE);
  expect_token(TOKEN_WHILE, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  current_node->while_loop.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->while_loop.body = parse_block(context, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  assert(current_node != NULL);
  return current_node;
}

node *parse_for(scope_context context, token **token_pointer) {
  node *current_node = create_node(NODE_FOR);
  expect_token(TOKEN_FOR, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  // `int i = 0;`
  if (is_type(peek_token(token_pointer)->value, context)) {
    current_node->for_loop.index_declaration = parse_type_expression(context, token_pointer);
  } else {
    current_node->for_loop.index_declaration = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
    expect_token(TOKEN_SEMI_COLON, token_pointer);
  }
  assert(current_node->for_loop.index_declaration->type == NODE_VARIABLE_DECLARATION);
  current_node->for_loop.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  assert(current_node->for_loop.condition->type == NODE_EQUATION);
  // `i++;`
  current_node->for_loop.index_assignment = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  assert(current_node->for_loop.index_assignment->equation.operator == OPERATOR_ASSIGN);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->for_loop.body = parse_block(context, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  assert(current_node != NULL);
  return current_node;
}

// if(condition) {}  (NO IF-ELSE STATEMENTS RN)
node *parse_if(scope_context context, token **token_pointer) {
  node *current_node = create_node(NODE_IF);
  expect_token(TOKEN_IF, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  current_node->if_statement.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->if_statement.success = parse_block(context, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  if (peek_token(token_pointer)->type == TOKEN_ELSE) {
    expect_token(TOKEN_ELSE, token_pointer);
    expect_token(TOKEN_LEFT_BRACE, token_pointer);
    current_node->if_statement.fail = parse_block(context, token_pointer);
    expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  } else {
    current_node->if_statement.fail = NULL;
  }
  assert(current_node != NULL);
  return current_node;
}

// Parses a block of tokens between (and excluding) braces, and turns it into an abstract syntax tree.
node *parse_block(scope_context context, token **token_pointer) {
  assert(vector_size((vector *)&context.typedef_hashmaps) > 0);
  assert(context.depth < UNREASONABLE_CONTEXT_NUMBER);
  assert(token_pointer != NULL);

  // Create new scope space
  context.depth += 1;
  create_or_clear_context(context);

  node *ast = create_node(NODE_BLOCK);
  ast->block.nodes = vector_create();
  node *current_node = NULL;
  token *current_token = peek_token(token_pointer);

  // Keep parsing individual statements until end of scope
  while (current_token->type != TOKEN_END && current_token->type != TOKEN_RIGHT_BRACE) {
    switch (current_token->type) {
    case TOKEN_END:
      expect_token(TOKEN_END, token_pointer);
      current_node = create_node(NODE_END);
      break;

    case TOKEN_TYPEDEF:
      parse_typedef(context, token_pointer);
      break;

    case TOKEN_STRUCT:
      current_node = parse_type_expression(context, token_pointer);
      expect_token(TOKEN_SEMI_COLON, token_pointer);
      break;

    case TOKEN_DO:
      current_node = parse_do_while(context, token_pointer);
      break;
    case TOKEN_WHILE:
      current_node = parse_while(context, token_pointer);
      break;
    case TOKEN_FOR:
      current_node = parse_for(context, token_pointer);
      break;

    case TOKEN_IF:
      current_node = parse_if(context, token_pointer);
      break;

    case TOKEN_LEFT_BRACE:
      expect_token(TOKEN_LEFT_BRACE, token_pointer);
      current_node = parse_block(context, token_pointer);
      break;
    default:
      if (is_type(current_token->value, context)) {
        current_node = parse_type_expression(context, token_pointer);
      } else {
        current_node = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
        expect_token(TOKEN_SEMI_COLON, token_pointer);
      }
      break;
    }
    assert(current_node != NULL);
    vector_add(&ast->block.nodes, current_node);
    current_token = peek_token(token_pointer);
  }

  return ast;
}

// Just puts a type into the type hashmap
void parse_typedef(scope_context context, token **token_pointer) {
  expect_token(TOKEN_TYPEDEF, token_pointer); 
  node *type = parse_type(context, token_pointer);

  typedef_entry entry = {
    .type = type,
    .name = expect_token(TOKEN_NAME, token_pointer)->value,
    .size_bytes = 8, // TODO: Calculate bytes by looking at all the base types inside of the type node
  };
  add_type_to_context(entry, context);
}

void print_indents(int indent_level) {
  for (int i = 0; i < indent_level; i++) {
    putchar('\t');
  }
}

void print_block(node *ast, int indent_level) {
  if (ast == NULL) {
    return;
  }

  print_indents(indent_level); printf("%s\n", node_type_to_string(ast->type));

  switch (ast->type) {
  default:
    error("Undefined node '%s'.", node_type_to_string(ast->type));
    break;
  case NODE_BLOCK:
    for (int i = 0; i < (int)vector_size((vector *)&ast->block.nodes); i++) {
      print_block(ast->block.nodes[i], indent_level + 1);
    }
    break;
  case NODE_STRING:
    print_indents(indent_level); printf(": \""); vector_print_string(&ast->string.value); printf("\"\n");
    break;
  case NODE_NUMBER_LITERAL:
    print_indents(indent_level); printf(": %d\n", ast->number_literal.value);
    break;
  case NODE_VARIABLE:
    print_indents(indent_level); printf(": "); vector_print_string(&ast->variable.name); printf("\n");
    break;
  case NODE_EQUATION:
    print_indents(indent_level); printf(": %s\n", operator_type_to_string(ast->equation.operator));

    print_block(ast->equation.left, indent_level + 1);
    print_block(ast->equation.right, indent_level + 1);
    break;
  case NODE_FUNCTION_CALL: 
    print_indents(indent_level); printf("Inputs:\n");
    for (int i = 0; i < (int)vector_size((vector *)&ast->function_call.inputs); i++) {
      print_block(ast->function_call.inputs[i], indent_level + 1);
    }

    print_indents(indent_level); printf("Body:\n"); 
    print_block(ast->function_call.function_expression, indent_level + 1);
    break;
  case NODE_WHILE:
    print_indents(indent_level); printf("Condition:\n"); 
    print_block(ast->while_loop.condition, indent_level + 1);
    print_indents(indent_level); printf("Body:\n"); 
    print_block(ast->while_loop.body, indent_level + 1);
    break;
  case NODE_DO_WHILE:
    print_indents(indent_level); printf("Condition:\n"); 
    print_block(ast->do_while_loop.condition, indent_level + 1);
    print_indents(indent_level); printf("Body:\n"); 
    print_block(ast->do_while_loop.body, indent_level + 1);
    break;
  case NODE_FOR:
    print_indents(indent_level); printf("Index Declaration:\n"); 
    print_block(ast->for_loop.index_declaration, indent_level + 1);
    print_indents(indent_level); printf("Condition:\n"); 
    print_block(ast->for_loop.condition, indent_level + 1);
    print_indents(indent_level); printf("Index Reassignment:\n"); 
    print_block(ast->for_loop.index_assignment, indent_level + 1);
    print_indents(indent_level); printf("Body:\n"); 
    print_block(ast->for_loop.body, indent_level + 1);
    break;
  case NODE_IF:
    print_indents(indent_level); printf("Condition:\n"); 
    print_block(ast->if_statement.condition, indent_level + 1);
    print_indents(indent_level); printf("Success Path:\n"); 
    print_block(ast->if_statement.success, indent_level + 1);
    if (ast->if_statement.fail != NULL) {
      print_indents(indent_level); printf("Fail Path:\n"); 
      print_block(ast->if_statement.fail, indent_level + 1);
    }
    break;
  case NODE_VARIABLE_DECLARATION:
    print_indents(indent_level); printf(": "); vector_print_string(&ast->variable_declaration.name); printf("\n");

    print_block(ast->variable_declaration.value, indent_level + 1);
    break;
  }
}


// Takes in tokens, outputs an Abstract Syntax Tree (AST)
node *parser(token *tokens) {
  // Check if token at end is end token.
  assert(((token *)vector_last(&tokens))->type == TOKEN_END);

  // Pointer to pointer so we can store state of which token we're on
  token **token_pointer = &tokens;

  scope_context context = {
    .typedef_hashmaps = vector_create(),
    .depth = 0,
  };

  create_or_clear_context(context);

  typedef_entry int_entry = {
    .name = _vector_from("int", sizeof(char), 4),
    .size_bytes = 4,
    .type = NULL,
  };
  typedef_entry char_entry = {
    .name = _vector_from("char", sizeof(char), 5),
    .size_bytes = 1,
    .type = NULL,
  }; 

  add_type_to_context(int_entry, context);
  add_type_to_context(char_entry, context);

  node *ast = parse_block(context, token_pointer);

  print_block(ast, 0);

  return ast;
}
