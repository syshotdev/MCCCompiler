#ifndef parser_h
#define parser_h
#include "enum_utilities.h"
#include "lexer.h"
#include <stdio.h>

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
  X(NODE_EXPRESSION)                                                           \
  X(NODE_EQUATION)                                                             \
  X(NODE_ADD)                                                                  \
  X(NODE_SUBTRACT)                                                             \
  X(NODE_MULTIPLY)                                                             \
  X(NODE_DIVIDE)                                                               \
                                                                               \
  X(NODE_COMPARISON)                                                           \
  X(NODE_NOT)                                                                  \
  X(NODE_AND)                                                                  \
  X(NODE_OR)                                                                   \
  X(NODE_XOR)                                                                  \
                                                                               \
  X(NODE_DECLARATION)                                                          \
  X(NODE_FUNCTION_DECLARATION)                                                 \
  X(NODE_PARAMETER)

typedef enum { ITERATE_NODES_AND(GENERATE_ENUM) } node_type;

extern const char *node_type_strings[];
const char *node_type_to_string(node_type type);

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

typedef struct node {
  node_type type;
  union {
    struct node *children; // Always a vector!
    char *data;            // Always a vector!
  };
} node;


typedef enum {
  TYPE_INT,
  TYPE_STRUCT,
  TYPE_ENUM,
} base_data_type;

// Literally types like int or char or struct node
typedef struct data_type {
  base_data_type type;
  union {
    struct data_type *children; // Always a vector!
    char *data;                 // Always a vector!
  };
} data_type;

typedef struct {
  char *name; // Always a vector!
} variable;

typedef struct {
  char *name; // Always a vector!
  data_type *parameters; // Always a vector!
  data_type *return_type; // Always a vector!
} function;

typedef void* hashmap;

typedef struct {
  hashmap data_types; // PLACEHOLDER HASHMAPS
  hashmap variables;
  hashmap functions;
  node *ast;
} ast;

node *parser(token *tokens);

#endif
