#ifndef parser_h
#define parser_h
#include "c-hashmap/hashmap.h"
#include "enum_utilities.h"
#include "lexer.h"
#include <stdio.h>

#define ITERATE_NODES_AND(X)                                                   \
  X(NODE_START)                                                                \
  X(NODE_END)                                                                  \
  X(NODE_NONE)                                                                 \
  X(NODE_BLOCK)                                                                \
                                                                               \
  X(NODE_STRING)                                                               \
  X(NODE_NUMBER_LITERAL)                                                       \
  X(NODE_VARIABLE)                                                             \
  X(NODE_STRUCTURE)                                                            \
  X(NODE_EQUATION)                                                             \
                                                                               \
  X(NODE_POINTER)                                                              \
  X(NODE_TYPE)                                                                 \
                                                                               \
  X(NODE_STRUCT_MEMBER_GET)                                                    \
  X(NODE_ARRAY_GET)                                                            \
                                                                               \
  X(NODE_IF)                                                                   \
  X(NODE_ELSEIF)                                                               \
  X(NODE_DO_WHILE)                                                             \
  X(NODE_WHILE)                                                                \
  X(NODE_FOR)                                                                  \
                                                                               \
  X(NODE_FUNCTION_CALL)                                                        \
  X(NODE_RETURN)                                                               \
                                                                               \
  X(NODE_VARIABLE_DECLARATION)                                                 \
  X(NODE_FUNCTION_DECLARATION)                                                 \
  X(NODE_PARAMETER)

// 'negate' is not same as 'not', it is for making numbers negative

#define ITERATE_OPERATORS_AND(X)                                               \
  X(OPERATOR_ASSIGN)                                                           \
  X(OPERATOR_ADD)                                                              \
  X(OPERATOR_SUBTRACT)                                                         \
  X(OPERATOR_MULTIPLY)                                                         \
  X(OPERATOR_DIVIDE)                                                           \
  X(OPERATOR_NEGATE)                                                           \
  X(OPERATOR_NOT)                                                              \
  X(OPERATOR_AND)                                                              \
  X(OPERATOR_OR)                                                               \
  X(OPERATOR_XOR)                                                              \
  X(OPERATOR_EQUALS_EQUALS)                                                    \
  X(OPERATOR_NOT_EQUALS)                                                       \
  X(OPERATOR_LESS_THAN)                                                        \
  X(OPERATOR_LESS_THAN_EQUALS)                                                 \
  X(OPERATOR_GREATER_THAN)                                                     \
  X(OPERATOR_GREATER_THAN_EQUALS)                                              \
  X(OPERATOR_BOOLEAN_AND)                                                      \
  X(OPERATOR_BOOLEAN_OR)                                                       \
  X(OPERATOR_DEREFERENCE)                                                      \
  X(OPERATOR_REFERENCE)

typedef enum { ITERATE_NODES_AND(GENERATE_ENUM) } node_type;
typedef enum { ITERATE_OPERATORS_AND(GENERATE_ENUM) } operator_type;

// 90% sure this "extern" is required. Remove at your own risk.
extern const char *node_type_strings[];
extern const char *operator_type_strings[];

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

typedef struct node **node_vector;
typedef struct hashmap **hashmap_vector;

typedef struct node {
  node_type type;
  union {
    struct {
      char_vector value;
    } string;
    struct {
      int value;
    } number_literal;
    struct {
      char_vector name;
    } variable;
    struct {
      char_vector name; // Optional (Nameless structs)
      node_vector members;
    } structure;

    // Operator is binary operator (+-*/%^&=) or unary operator
    // a and b are statements
    struct {
      operator_type operator;
      struct node *left;
      struct node *right; // Optional based on whether a binary or unary operation
    } equation;

    // Type-building nodes
    // This refers to typedef map, with types like "int"
    struct {
      char_vector name;
      // bool is_constant;
    } base_type;
    struct {
      struct node *from;
    } pointer;

    // A variable declared from a type
    struct {
      struct node *type; // Type node (Required)
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
      struct node *condition; // TODO: How do I implement else ifs???
      struct node *success;
      struct node *fail; // Else path (Optional)
    } if_statement;
    struct {
      struct node *body;
      struct node *condition;
    } do_while_loop;
    struct {
      struct node *condition;
      struct node *body;
    } while_loop;
    struct {
      struct node *index_declaration;
      struct node *condition;
      struct node *index_assignment;
      struct node *body;
    } for_loop;
    struct {
      struct node *type;
      char_vector name;
      node_vector parameters;
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
  char_vector name;
  node *type_expression;
  int size_bytes;
} typedef_entry;

typedef struct {
  hashmap_vector typedef_hashmaps;
  vec_size_t depth; // It is mainly used with vectors so vec_size_t for squashing warnings
} scope_context;

// FUNCTION PROTOTYPES
// Helpful debugging functions
const char *node_type_to_string(node_type type);
const char *operator_type_to_string(operator_type type);

// Base functions
void advance_token(token **token_pointer);
void return_token(token **token_pointer);
token *pop_token(token **token_pointer);
token *peek_token(token** token_pointer);
token *expect_token(token_type type, token **token_pointer);
bool is_at_end(token **token_pointer);
node *create_node(node_type type);

// Types
int compare_typedef_entries(const void *a, const void *b, void *udata);
uint64_t hash_typedef_entry(const void *data, uint64_t seed0, uint64_t seed1);
void create_or_clear_context(scope_context context);
void add_type_to_context(typedef_entry *object, scope_context context);
bool is_type(char_vector name, scope_context context);
node_vector collect_members(token_type right_break_token, scope_context context, token **token_pointer);
node *parse_base_type(scope_context context, token **token_pointer);
node *parse_structure_type(scope_context context, token **token_pointer);
node *parse_type(scope_context context, token **token_pointer);
node *parse_type_expression(scope_context context, token **token_pointer);

// Parsing helpers
precedence get_precedence(token_type type);
operator_type get_binary_type(token_type type);
bool should_parse_unary(token_type type);
node *create_equation_equals_equation_and_next(node *last_expression, node *next_expression, operator_type operator);
node *create_struct_member_get(node *from_expression, token **token_pointer);

// Parsing
node *parse_string(token **token_pointer);
node *parse_number(token **token_pointer);
node *parse_variable_expression(token **token_pointer);
node *parse_struct_member_get(node *from_expression, token **token_pointer);
node *parse_struct_member_dereference_get(node *from_expression, token **token_pointer);
node *parse_function_call(node *from_expression, token **token_pointer);
node *parse_grouping(token **token_pointer);
node *parse_binary(node *last_expression, token **token_pointer);
node *parse_unary(token **token_pointer);
node *parse_postfix(node *last_expression, token **token_pointer);
node *switch_expression(node *last_expression, node *current_expression, token_type type, token **token_pointer);
node *parse_expression(precedence precedence, token **token_pointer);
node *parse_function(node *function_expression, scope_context context, token **token_pointer);
node *parse_do_while(scope_context context, token **token_pointer);
node *parse_while(scope_context context, token **token_pointer);
node *parse_for(scope_context context, token **token_pointer);
node *parse_if(scope_context context, token **token_pointer);
node *parse_block(scope_context context, token **token_pointer);
void parse_typedef(scope_context context, token **token_pointer);

// Parsing visualizers
void print_indents(int indent_level);
void print_block(node *ast, int indent_level);

// Main function
node *parser(token *tokens);

#endif
