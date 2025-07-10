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
  const char_vector string1 = *(char_vector*)a;
  const char_vector string2 = *(char_vector*)b;
  return strncmp(string1, string2, vector_size((vector *)&string1));
}

uint64_t hash_string_vector(const void *data, uint64_t seed0, uint64_t seed1) {
  const char_vector string = *(char_vector*)data;
  const size_t char_vector_size = vector_size((vector *)&string);
  return hashmap_sip(string, sizeof(char) * char_vector_size, seed0, seed1);
}

void create_or_clear_context(scope_context *context, int depth) {
  if (vector_has((vector *)&context->type_hashmaps, depth)) {
    struct hashmap *type_hashmap = *(struct hashmap**)vector_get(&context->type_hashmaps, depth);
    hashmap_clear(type_hashmap, true);
  } else {
    struct hashmap *type_hashmap = hashmap_new(sizeof(char_vector), 1, 0, 0, hash_string_vector, compare_string_vectors, NULL, NULL);
    vector_insert(&context->type_hashmaps, depth, type_hashmap);
    assert(*(struct hashmap**)vector_get(&context->type_hashmaps, depth) == type_hashmap);
  }
}

void add_type_to_context(scope_context *context, char_vector type_name, int depth) {
  // Just a reminder, 'hashmap_set' takes in hashmap and object.
  // Object has the key (usually object.name), and itself as the value.
  // That is why it is only the 'key' instead of 'key:value'
  hashmap_set(context->type_hashmaps[depth], &type_name);
  assert(hashmap_get(context->type_hashmaps[depth], &type_name) != NULL);
}

bool is_type(scope_context *context, char_vector name) {
  hashmap_vector type_hashmaps = context->type_hashmaps;
  assert(type_hashmaps[0] == context->type_hashmaps[0]);
  for (vec_size_t i = 0; i < vector_size((vector *)&type_hashmaps); i++) {
    if (hashmap_get(type_hashmaps[i], &name) != NULL) {
      return true;
    } else {
      printf("Info: '%.*s' isn't in context/type hashmaps\n", (int)vector_size((vector*)&name), name);
    }
  }
  return false;
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

// Prototype because it complains about multiple function definitions (It's used
// before it's defined).
node *parse_block(scope_context *context, int scope, token **token_pointer);
node *parse_expression(precedence precedence, token **token_pointer);
node *parse_variable_expression(token **token_pointer);

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

  node *variable_expression = create_node(NODE_NONE);
  // Set the type_info and name for variables AND functions AND parameters
  variable_expression->variable_declaration.type = type;
  variable_expression->variable_declaration.name = name;

  return variable_expression;
}

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

node *parse_string(token **token_pointer) {
  node *string_node = create_node(NODE_STRING);
  // Get value from that token that sent us here
  string_node->string.value = pop_token(token_pointer)->value;
  return string_node;
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
  node *expression = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  assert(expression != NULL);
  return expression;
}

operator_type token_type_to_binary_operator_type(token_type type) {
  operator_type operator;
  switch (type) {
  // All of these technically reassign a variable, but ++ and +=
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
    operator = OPERATOR_AND_AND;
    break;
  case TOKEN_OR:
    operator = OPERATOR_OR_OR;
    break;
  default:
    // TODO: Error
    printf("Error when parsing binary: Unknown operator '%s'\n", token_type_to_string(type));
    operator = OPERATOR_ADD;
    break;
  }
  return operator;
}

node *equation_equals_equation_and_next(node *last_expression, node *next_expression, operator_type operator, token** token_pointer) {
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

node *parse_binary(node *last_expression, token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  operator_type operator = token_type_to_binary_operator_type(operator_token->type);

  node *next_expression = parse_expression(get_precedence(operator_token->type) + 1, token_pointer);
  
  assert(last_expression != NULL);
  assert(next_expression != NULL);
  assert(operator_token->type < 1000);

  node *current_node = create_node(NODE_EQUATION);

  switch (operator_token->type) {
  case TOKEN_PLUS_EQUALS:
    current_node = equation_equals_equation_and_next(last_expression, next_expression, 
        OPERATOR_ADD, token_pointer);
    break;
  case TOKEN_MINUS_EQUALS:
    current_node = equation_equals_equation_and_next(last_expression, next_expression, 
        OPERATOR_SUBTRACT, token_pointer);
    break;
  default:
    current_node->equation.operator = operator;
    current_node->equation.left = last_expression;
    current_node->equation.right = next_expression;
    break;
  }

  return current_node;
}


// variable.function_pointer(expression, expression)
// We don't care about what's after this call
node *parse_function_call(node *from_expression, token **token_pointer) {
  assert(from_expression != NULL);

  node *current_node = create_node(NODE_FUNCTION_CALL);
  current_node->function_call.function_expression = from_expression;
  current_node->function_call.inputs = vector_create();

  // Loop till end or TOKEN_RIGHT_PARENTHESES
  // Example: func(1 + 5 + a, "string");
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

node *create_struct_member_get(node *from_expression, token **token_pointer) {
  node *current_node = create_node(NODE_STRUCT_MEMBER_GET);
  current_node->struct_member_get.from = from_expression;
  // Get the name from the next token, that is the member we're trying to get
  current_node->struct_member_get.name = pop_token(token_pointer)->value;
  return current_node;
}

// variable.member <- This last part
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

// These next three functions I have no idea what they do
node *parse_variable_assignment(token **token_pointer) {
  node *variable_expression = create_node(NODE_VARIABLE_DECLARATION);
  // TODO: Figure out whether to make new node or roll with 'no type'
  // current_node->variable.type = NULL;
  // Type is type_info btw for future reference
  variable_expression->variable_declaration.name = expect_token(TOKEN_NAME, token_pointer)->value;
  expect_token(TOKEN_EQUALS, token_pointer);
  variable_expression->variable_declaration.value = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  return variable_expression;
}

node *parse_variable_declaration(node *variable_expression, token **token_pointer) {
  expect_token(TOKEN_EQUALS, token_pointer);
  variable_expression->type = NODE_VARIABLE_DECLARATION;
  variable_expression->variable_declaration.value = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  return variable_expression;
}

// `int i;`
node *parse_variable_prototype(node *variable_expression, token **token_pointer) {
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  variable_expression->type = NODE_VARIABLE_DECLARATION;
  variable_expression->variable_declaration.value = NULL;
  return variable_expression;
}

// `i->foop.function()`
node *parse_variable_expression(token **token_pointer) {
  node *current_expression = create_node(NODE_VARIABLE);
  current_expression->variable.name = pop_token(token_pointer)->value;

  bool get_out_of_loop = false;
  while(get_out_of_loop == false) {
    token *operator_token = peek_token(token_pointer);
    switch (operator_token->type){
    default:
      // If it isn't any operator, get out of the loop
      get_out_of_loop = true;
      break;
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
  return current_expression;
}

node *parse_unary(token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  operator_type operator = OPERATOR_NEGATE;
  switch (operator_token->type) {
  default:
    // TODO: Error
    printf("Error when parsing unary: Unknown operator '%s'\n", token_type_to_string(operator_token->type));
    return parse_expression(PRECEDENCE_UNARY, token_pointer);
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
    printf("Error when parsing postfix: Unknown operator '%s'\n", token_type_to_string(operator_token->type));
    break;
  case TOKEN_PLUS_PLUS:
    operator = OPERATOR_ADD;
    break;
  case TOKEN_MINUS_MINUS:
    operator = OPERATOR_SUBTRACT;
    break;
  }
  // 1
  node *one = create_node(NODE_NUMBER_LITERAL);
  one->number_literal.value = 1;

  // i + 1
  node *plus_expression = create_node(NODE_EQUATION); // Idk what to name this
  plus_expression->equation.operator = operator;
  plus_expression->equation.left = last_expression;
  plus_expression->equation.right = one;

  // i = i + 1
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
    assert("Error in switch_expression: This path isn't supposed to be reached." == NULL);
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

  // Imagine a scenario like this: (get_object()->number++) + 5
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

node *parse_function(node *function_expression, token **token_pointer) {
  function_expression->function.parameters = vector_create();
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
    vector_add(&function_expression->function.parameters, current_parameter);
    assert(((parameter *)vector_last(&function_expression->function.parameters))->name == current_parameter.name);
    // If we parse expression and token on right is anything but comma or parentheses, syntax error
    if (peek_token(token_pointer)->type != TOKEN_RIGHT_PARENTHESES) {
      expect_token(TOKEN_COMMA, token_pointer);
    }
  }
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  return function_expression;
}

// Parse a variable or function declaration.
node *parse_type(scope_context *context, int depth, token **token_pointer) {
  assert(context != NULL);

  node *current_node = parse_variable_attributes(token_pointer);
  token_type type = peek_token(token_pointer)->type;
  switch (type) {
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
    // TODO: Error
    printf("Error when parsing type: Unknown token '%s'\n", token_type_to_string(type));
    return current_node;
  }

  assert(current_node != NULL);
  assert(current_node->type != NODE_NONE);
  return current_node;
}

node *parse_do_while(scope_context *context, int depth, token **token_pointer) {
  node *current_node = create_node(NODE_DO_WHILE);
  expect_token(TOKEN_DO, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->do_while_loop.body = parse_block(context, depth + 1, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  expect_token(TOKEN_WHILE, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  current_node->do_while_loop.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  assert(current_node != NULL);
  return current_node;
}
node *parse_while(scope_context *context, int depth, token **token_pointer) {
  node *current_node = create_node(NODE_WHILE);
  expect_token(TOKEN_WHILE, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  current_node->while_loop.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->while_loop.body = parse_block(context, depth + 1, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  assert(current_node != NULL);
  return current_node;
}
node *parse_for(scope_context *context, int depth, token **token_pointer) {
  node *current_node = create_node(NODE_FOR);
  expect_token(TOKEN_FOR, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  // int i = 0;
  if (is_type(context, peek_token(token_pointer)->value)) {
    current_node->for_loop.index_declaration = parse_type(context, depth, token_pointer);
  } else {
    current_node->for_loop.index_declaration = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
    expect_token(TOKEN_SEMI_COLON, token_pointer);
  }
  assert(current_node->for_loop.index_declaration->type == NODE_VARIABLE_DECLARATION);
  // TODO: Change parse_variable_declaration to require you to remove semicolon.
  // This also requires changing other similar functions, which is why I will ignore it for now.
  // Also TODO: Variable declarations are now counted as expressions, or at least dealt with in the expression parser i < 5;
  current_node->for_loop.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_SEMI_COLON, token_pointer);
  assert(current_node->for_loop.condition->type == NODE_EQUATION);
  // i++;
  current_node->for_loop.index_assignment = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  assert(current_node->for_loop.index_assignment->equation.operator == OPERATOR_ASSIGN);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->for_loop.body = parse_block(context, depth + 1, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  assert(current_node != NULL);
  return current_node;
}

// if(condition) {}  (NO IF-ELSE STATEMENTS RN)
node *parse_if(scope_context *context, int depth, token **token_pointer) {
  node *current_node = create_node(NODE_IF);
  expect_token(TOKEN_IF, token_pointer);
  expect_token(TOKEN_LEFT_PARENTHESES, token_pointer);
  current_node->if_statement.condition = parse_expression(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token(TOKEN_RIGHT_PARENTHESES, token_pointer);
  expect_token(TOKEN_LEFT_BRACE, token_pointer);
  current_node->if_statement.success = parse_block(context, depth + 1, token_pointer);
  expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  if (peek_token(token_pointer)->type == TOKEN_ELSE) {
    expect_token(TOKEN_ELSE, token_pointer);
    expect_token(TOKEN_LEFT_BRACE, token_pointer);
    current_node->if_statement.fail = parse_block(context, depth + 1, token_pointer);
    expect_token(TOKEN_RIGHT_BRACE, token_pointer);
  } else {
    current_node->if_statement.fail = NULL;
  }
  assert(current_node != NULL);
  return current_node;
}

// Parses a block of tokens between (and excluding) braces, and turns it into an abstract syntax tree.
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
  while (current_token->type != TOKEN_END && current_token->type != TOKEN_RIGHT_BRACE) {
    switch (current_token->type) {
    case TOKEN_END:
      // Add ending notifier
      expect_token(TOKEN_END, token_pointer);
      current_node = create_node(NODE_END);
      break;

    case TOKEN_TYPEDEF:
      expect_token(TOKEN_TYPEDEF, token_pointer);
      add_type_to_context(context, current_token->value, depth);
      break;

    // TODO: Implement structs
    // Probably need to add variables to the scope context (finally),
    // Each struct will just be a (type? variable? new struct type?)
    // with some members, each of those having a type (int), pointer amount,
    // and a name.
    // Idk, it should be recursive, and easy to parse structs in structs.
    // However, maybe that's too complicated and would require a very large
    // refactor, so if it aint' possible just do the bare minimums.
    // Good luck future me!
    case TOKEN_STRUCT:
      expect_token(TOKEN_STRUCT, token_pointer);
      add_type_to_context(context, current_token->value, depth);
      break;

    case TOKEN_DO:
      current_node = parse_do_while(context, depth, token_pointer);
      break;
    case TOKEN_WHILE:
      current_node = parse_while(context, depth, token_pointer);
      break;
    case TOKEN_FOR:
      current_node = parse_for(context, depth, token_pointer);
      break;

    case TOKEN_IF:
      current_node = parse_if(context, depth, token_pointer);
      break;

    case TOKEN_LEFT_BRACE:
      expect_token(TOKEN_LEFT_BRACE, token_pointer);
      current_node = parse_block(context, depth + 1, token_pointer);
      break;
    default:
      // If token is none of these, it is expression or variable declaration.
      // First: check if is a type (int), if not it is an expression. If error, expression will catch it.
      if (is_type(context, current_token->value)) {
        current_node = parse_type(context, depth, token_pointer);
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

  // Requires: access tokens, context info (types, ...), depth/indentation/scope
  // typedef int myint;
  // myint i = 0;
  scope_context context = {
    .type_hashmaps = vector_create(),
    .var_hashmaps = vector_create(),
  };

  create_or_clear_context(&context, 0);
  add_type_to_context(&context, _vector_from("int\0", sizeof(char), 4), 0);
  add_type_to_context(&context, _vector_from("int\0", sizeof(char), 4), 0);

  // Parse scope (with a scope number of one, we declared int for 0'th layer)
  node *ast = parse_block(&context, 1, token_pointer);

  print_block(ast, 0);

  return ast;
}
