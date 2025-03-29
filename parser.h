#ifndef parser_h
#define parser_h
#include "c-hashmap/hashmap.h"
#include "enum_utilities.h"
#include "lexer.h"
#include <stdio.h>

// TODO: Should I use "loops" and "if"s instead of "while" and "for" and "elif"?
// Basically, generic names over very specific ones? We could just expand the
// for statement to equal a while loop and I think it would work fine.
#define ITERATE_NODES_AND(X)                                                   \
  X(NODE_START)                                                                \
  X(NODE_END)                                                                  \
  X(NODE_NONE)                                                                 \
  X(NODE_BLOCK)                                                                \
                                                                               \
  X(NODE_NUMBER_LITERAL)                                                       \
  X(NODE_VARIABLE)                                                             \
  X(NODE_EQUATION)                                                             \
                                                                               \
  X(NODE_STRUCT_MEMBER_GET)                                                    \
  X(NODE_ARRAY_GET)                                                            \
                                                                               \
  X(NODE_IF)                                                                   \
  X(NODE_ELSEIF)                                                               \
  X(NODE_FOR)                                                                  \
  X(NODE_WHILE)                                                                \
                                                                               \
  X(NODE_CALL_FUNCTION)                                                        \
  X(NODE_RETURN)                                                               \
                                                                               \
  X(NODE_VARIABLE_DECLARATION)                                                 \
  X(NODE_FUNCTION_DECLARATION)                                                 \
  X(NODE_PARAMETER)

// 'negate' is not same as 'not', is for making number negative

#define ITERATE_OPERATORS_AND(X)                                               \
  X(OPERATOR_ADD)                                                              \
  X(OPERATOR_SUBTRACT)                                                         \
  X(OPERATOR_MULTIPLY)                                                         \
  X(OPERATOR_DIVIDE)                                                           \
  X(OPERATOR_NEGATE)                                                           \
  X(OPERATOR_NOT)                                                              \
  X(OPERATOR_AND)                                                              \
  X(OPERATOR_OR)                                                               \
  X(OPERATOR_XOR)                                                              \
  X(OPERATOR_DEREFERENCE)                                                      \
  X(OPERATOR_REFERENCE)                                                        \

typedef enum { ITERATE_NODES_AND(GENERATE_ENUM) } node_type;
typedef enum { ITERATE_OPERATORS_AND(GENERATE_ENUM) } operator_type;

// 90% sure this "extern" is required. Remove at your own risk.
extern const char *node_type_strings[];
extern const char *operator_type_strings[];

// Call this function to get string from node type
const char *node_type_to_string(node_type type);
const char *operator_type_to_string(operator_type type);

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

// A type, like 'int'
// Includes: name, type size, what it contains (char[30] name, int id)
typedef struct {
  char_vector name;
  int pointer_amount;
} type_info;
// Parameter could also be '...', but I'm not sure how to implement that
typedef struct {
  type_info type;
  char_vector name;
} parameter;

typedef parameter *parameter_vector;
typedef struct node **node_vector;
typedef struct hashmap **hashmap_vector;

typedef struct node {
  node_type type;
  union {
    struct {
      int value;
    } number_literal;
    struct {
      char_vector name;
    } variable;
    struct {
      char_vector name;
      parameter_vector members;
    } structure;
    // Operator is binary operator (+-*/%^&) or unary operator
    // a and b are statements
    struct {
      operator_type operator;
      struct node *left;
      struct node *right; // Optional based on whether a binary or unary operation
    } equation;
    struct {
      type_info type;
      char_vector name;
      struct node *value; // Equation after the equals (=) sign (Optional)
    } variable_declaration;
    struct {
      char_vector name;
      struct node *from;
    } struct_member_get;
    struct {
      struct node *index_expression;
      struct node *from;
    } array_get;
    struct {
      struct node *condition;
      struct node *success;
      struct node *fail; // Else path (Optional)
    } if_statement;
    struct {
      struct node *condition;
      struct node *body;
    } loop;
    struct {
      type_info type;
      char_vector name;
      parameter_vector parameters;
      struct node *body;
    } function;
    struct {
      struct node *function_expression;
      node_vector inputs;
    } function_call;
    struct {
      node_vector nodes;
    } block;
  };
} node;

typedef struct {
  hashmap_vector type_hashmaps;
  hashmap_vector var_hashmaps;
} scope_context;

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
