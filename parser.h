#ifndef parser_h
#define parser_h
#include "c-hashmap/hashmap.h"
#include "enum_utilities.h"
#include "lexer.h"
#include <stdio.h>


typedef enum {
  PRECEDENCE_NONE,
  PRECEDENCE_ASSIGNMENT, // =
  PRECEDENCE_OR,         // or
  PRECEDENCE_AND,        // and
  PRECEDENCE_EQUALITY,   // == !=
  PRECEDENCE_COMPARISON, // < > <= >=
  PRECEDENCE_TERM,       // + -
  PRECEDENCE_FACTOR,     // * /
  PRECEDENCE_UNARY,      // ! -
  PRECEDENCE_CALL,       // . ()
  PRECEDENCE_PRIMARY,
} precedence;

#define ITERATE_NODES_AND(X)                                                   \
  X(NODE_START)                                                                \
  X(NODE_END)                                                                  \
  X(NODE_NONE)                                                                 \
                                                                               \
  X(NODE_CALL_FUNCTION)                                                        \
  X(NODE_RETURN)                                                               \
                                                                               \
  X(NODE_SEPARATOR)                                                            \
                                                                               \
  X(NODE_POINTER)                                                              \
  X(NODE_TYPE)                                                                 \
  X(NODE_VARIABLE)                                                             \
  X(NODE_NUMBER)                                                               \
  X(NODE_STRING)                                                               \
                                                                               \
  X(NODE_EQUATION)                                                             \
  X(NODE_ADD)                                                                  \
  X(NODE_SUBTRACT)                                                             \
  X(NODE_MULTIPLY)                                                             \
  X(NODE_DIVIDE)                                                               \
                                                                               \
  X(NODE_COMPARISON)                                                           \
  X(NODE_NEGATE)                                                               \
  X(NODE_NOT)                                                                  \
  X(NODE_AND)                                                                  \
  X(NODE_OR)                                                                   \
  X(NODE_XOR)                                                                  \
                                                                               \
  X(NODE_VARIABLE_DECLARATION)                                                 \
  X(NODE_FUNCTION_DECLARATION)                                                 \
  X(NODE_PARAMETER)

typedef enum { ITERATE_NODES_AND(GENERATE_ENUM) } node_type;

// 90% sure this "extern" is required. Remove at your own risk.
extern const char *node_type_strings[];
// Call this function to get string from node type
const char *node_type_to_string(node_type type);


/*
struct string {
  char_vector value;
}
struct number {
  int value;
}
*/
// Operator is binary operator (+-*/%^&) or unary operator
// a and b are statements
/*
struct equation {
  struct token_type operator;
  struct statement a;
  struct statement b; // Optional based on whether a binary or unary operation
}
struct variable {
  type;
  pointer_amount;
  name;
  value; // (Statement or maybe Equation?) (Optional)
}
struct statement {
  // is equal to variable declaration/assignment, math operators, conditions, function call, etc.
  // Statements are used everywhere, for example in while loops or for loops, or
  // declaring a variable in a function. Idk what a statement would have, though.
}
struct function {
  type;
  pointer_amount;
  name;
  parameters;
  block;
}
// Parameter could also be '...', but I'm not sure how to implement that yet.
struct parameter {
  type;
  pointer_amount;
  name;
}
struct function_call {
  name;
  parameters;
}
*/
typedef struct node {
  node_type type;
  struct node *children; // Always a vector!
  char *data;            // Always a vector!
} node;



typedef struct hashmap** hashmap_vector;

// ALL OF THESE WILL GO INTO NEXT COMPILATION STEP
// Literally types like int or char* or struct node
/*
typedef struct data_type {
  int size_bytes;
  struct data_type *children; // Always a vector!
} data_type;

typedef struct {
  data_type type;
  int pointers; // Is this pointer, if so, how much
} variable;

typedef struct {
  data_type *parameters;  // Always a vector!
  data_type *return_type; // Always a vector!
} function;

typedef struct {
  hashmap *data_types;
  hashmap *variables;
  hashmap *functions;
  node *ast;
} ast; */

node *parser(token *tokens);

#endif
