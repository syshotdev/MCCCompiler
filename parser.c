#include "parser.h"
#include "c-vector/vec.h"
#include <stdio.h>
#include <stdlib.h>

const char *node_type_strings[] = {ITERATE_NODES_AND(GENERATE_STRING)};

const char *node_type_to_string(node_type type) {
  return enum_to_string(type, node_type_strings);
}

node *create_node(node_type type) {
  node *node = malloc(sizeof(node));
  node->type = type;
  node->children = vector_create();
  node->data = vector_create();
  return node;
}

// Same thing as create_node but *data dereferenced and put into new vector
node *create_node_with_data(node_type type, char *data) {
  node *node = malloc(sizeof(node));
  node->type = type;
  node->children = vector_create();
  node->data = vector_create();
  // Bug here. Add entire vector lol
  vector_add(&node->data, *data);
  return node;
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
  token *token = &(**token_pointer);
  advance_token(token_pointer);
  return token;
}

token *peek_token(token **token_pointer) {
  token *token = &(**token_pointer);
  return token;
}

bool is_at_end(token **token_pointer) {
  return peek_token(token_pointer)->type == TOKEN_END;
}

token *expect_token(token_type type, token **token_pointer) {
  token *token = pop_token(token_pointer);
  if (token->type != type) {
    printf("Token given: %s \n Token expected: %s \n",
           token_type_to_string(token->type), token_type_to_string(type));
  }
  return token;
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
  node *number_node = create_node(NODE_NUMBER);
  // Get value from that token that sent us here
  int value = atoi(pop_token(token_pointer)->value);
  vector_add(&number_node->data, value);

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

  node_type type;
  switch (operator_token->type) {
  case TOKEN_PLUS:
    type = NODE_ADD;
    break;
  case TOKEN_MINUS:
    type = NODE_SUBTRACT;
    break;
  case TOKEN_STAR:
    type = NODE_MULTIPLY;
    break;
  case TOKEN_SLASH:
    type = NODE_DIVIDE;
    break;
  default:
    printf("Error in parse binary");
    exit(1);
    break;
  }

  node *ast = create_node(NODE_EQUATION);
  vector_reserve(&ast->children, 3);
  vector_add(&ast->children, *create_node(type));
  vector_add(&ast->children, *last_expression);
  vector_add(&ast->children, *next_expression);

  return ast;
}

node *parse_unary(token **token_pointer) {
  token *operator_token = pop_token(token_pointer);
  node *expression = parse_precedence(PRECEDENCE_UNARY, token_pointer);

  node_type type;
  switch (operator_token->type) {
  // +NUMBER does nothing
  case TOKEN_PLUS:
    return expression;
    break;
  case TOKEN_MINUS:
    type = NODE_NEGATE;
    break;
  case TOKEN_NOT:
    type = NODE_NOT;
    break;
  default:
    printf("Error in parse unary");
    exit(1);
    break;
  }

  node *ast = create_node(NODE_EQUATION);
  ast->children = vector_create();
  vector_reserve(&ast->children, 2);
  vector_add(&ast->children, *create_node(type));
  vector_add(&ast->children, *expression);

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

  node *ast;
  node *last_expression;
  node *current_expression;

  // Keep parsing equal or higher precedence values, or ';' is found.
  // Example: -12 - -6 * (2 + 3);
  while (precedence <= get_precedence(current_token->type)) {
    switch (current_token->type) {
    case TOKEN_SEMI_COLON:
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

    // Alright, Idk if this is even the way to do it. Refactor a lot of this and
    // get it working!
    ast = current_expression;
  }

  return ast;
}

// Wrapper for parse precedence
node *parse_expression(token **token_pointer) {
  return parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
}

int how_many_in_a_row(token_type type, token **token_pointer) {
  int number = 0;
  while (true) {
    if (peek_token(token_pointer)->type != type) {
      break;
    }
    // Pop * token
    pop_token(token_pointer);
    number++;
  }
  return number;
}

// Parse something like "a = 12 + (9 * 20);"
node *parse_variable_declaration(token **token_pointer) {
  token *name = expect_token(TOKEN_NAME, token_pointer);
  expect_token(TOKEN_EQUALS, token_pointer);
  node *ast = create_node(NODE_VARIABLE_DECLARATION);
  parse_expression(token_pointer);
  return ast;
}

// int sum_ints(int *array) {...}
node *parse_function(token **token_pointer) { return create_node(NODE_NONE); }

// Parse a variable or function declaration.
node *parse_type(token **token_pointer) {
  token *type = expect_token(TOKEN_NAME, token_pointer);
  int pointer_amount = how_many_in_a_row(TOKEN_STAR, token_pointer);
  token *name = expect_token(TOKEN_NAME, token_pointer);

  token *current_token = peek_token(token_pointer);
  node *ast = create_node(NODE_NONE);
  vector_reserve(&ast->children, 4);

  // Oh man so messy
  node *node_type = create_node_with_data(NODE_TYPE, type->value);
  node *node_pointer =
      create_node_with_data(NODE_POINTER, (char *)(&pointer_amount));
  node *node_variable = create_node_with_data(NODE_VARIABLE, name->value);

  vector_add(&ast->children, *node_type);
  vector_add(&ast->children, *node_pointer);
  vector_add(&ast->children, *node_variable);

  switch (current_token->type) {
  case TOKEN_EQUALS:
    expect_token(TOKEN_EQUALS, token_pointer);
    ast->type = NODE_VARIABLE_DECLARATION;
    vector_add(&ast->children, *parse_expression(token_pointer));
    break;
  case TOKEN_SEMI_COLON:
    printf("Error in parse type, didn't implement un-initialized variables\n");
    break;
  case TOKEN_LEFT_PARENTHESES:
    ast->type = NODE_FUNCTION_DECLARATION;
    ast = parse_function(token_pointer);
    break;
  default:
    // Error
    return ast;
  }

  return ast;
}

// Checks if has two name tokens in a row, example:
// int variable = ...
bool is_token_type(token **token_pointer) {
  pop_token(token_pointer);
  token *current_token = peek_token(token_pointer);

  bool is_token_type = false;
  if (current_token->type == TOKEN_NAME) {
    is_token_type = true;
  }
  return_token(token_pointer);
  return is_token_type;
}

// Parses a block of C tokens and outputs the block's AST reprisentation.
node *parse_block(token **token_pointer) {
  token *current_token = peek_token(token_pointer);
  node *ast;

  //printf("Current token: '%s'\n",
  //         token_type_to_string(current_token->type));

  switch (current_token->type) {
  default:
    //printf("Error in parse block, unknown type '%s'\n",
    //       token_type_to_string(current_token->type));
    exit(1);
    break;

  case TOKEN_NAME:
    if (is_token_type(token_pointer)) {
      ast = parse_type(token_pointer);
    } else {
      ast = parse_variable_declaration(token_pointer);
    }
    break;

  case TOKEN_END:
    ast = create_node(NODE_END);
    break;
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
  char *spaces = malloc(indent_level * sizeof(char));
  for (int i = 0; i < indent_level; i++) {
    spaces[i] = ' ';
  }

  // Print out this node
  printf("%s%s\n", spaces, node_type_to_string(ast->type));
  for (int i = 0; i < vector_size(ast->children); i++) {
    print_ast(&ast->children[i], indent_level + 1);
  }

  // TODO: print out the value of this node
}

// Takes in tokens, outputs an Abstract Syntax Tree (AST)
node *parser(token *tokens) {
  // Pointer to pointer so we can store state of which token we're on
  token **token_pointer = &tokens;
  node *ast = parse_block(token_pointer);

  print_ast(ast, 0);

  return ast;
}
