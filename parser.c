->#include "c-vector/vec.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * The syntax function should take a look at the tree and figure out if it's
 * grammatically correct Semantic analyzation is checking if parameters have the
 * right types, variables are initialized, stuff like that.
 *
 */

typedef enum {
  NODE_START,
  NODE_END,
  NODE_NONE,

  NODE_CALL_FUNCTION,
  NODE_RETURN,

  NODE_SEPARATOR,

  NODE_POINTER,
  NODE_TYPE,
  NODE_VARIABLE,
  NODE_LITERAL,
  NODE_STRING,

  NODE_EQUATION,
  NODE_ADD,
  NODE_SUBTRACT,
  NODE_MULTIPLY,
  NODE_DIVIDE,

  NODE_DECLARATION,
  NODE_FUNCTION_DECLARATION,
  NODE_PARAMETER,
} node_type;

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

typedef struct {
  node_type type;
  union {
    struct node *next_nodes; // Always a vector!
    char *data;              // Always a vector!
  };
} node;

node *generate_node(node_type type) {
  node *node = malloc(sizeof(node));
  node->type = type;
  node->next_nodes = vector_create();
  return node;
}

void backtrack_token(token **token_pointer) {
  // Pointer arithmetic to backtrack pointer by 1 token length
  *token_pointer -= 1;
}
void advance_token(token **token_pointer) {
  // Pointer arithmetic to advance pointer by 1 token length
  *token_pointer += 1;
}

token *pop_token(token **token_pointer) {
  token *token = &(**token_pointer);
  advance_token(token_pointer);
  return token;
}

token *peek_token(token **token_pointer) {
  token *token = &(*token_pointer[1]);
  return token;
}

bool is_at_end(token **token_pointer) {
  return peek_token(token_pointer)->type == TOKEN_END;
}

token *expect_token(token_type type, token **token_pointer) {
  token *token = pop_token(token_pointer);
  if (token->type != type) {
    // TODO: Make the enums actually exist as words instead of compiler-time
    // numbers
    printf("Token given: %i \n Token expected: %i \n", token->type, type);
    exit(1);
  }
  return token;
}
// Same thing as expect_token, but checks if value[0] == expected char
token *expect_token_value(token_type type, char expected,
                          token **token_pointer) {
  token *token = pop_token(token_pointer);
  // Token type or token value is not expected, send error
  if (token->type != type || token->value[0] != expected) {
    printf("Token given: %i \nToken expected: %i \nChar expected: %c\n",
           token->type, type, expected);
    exit(1);
  }
  return token;
}

void syntax_analyzer() {}
void semantic_analyzer() {}

/*
 * int i = 1;
 *
 * TYPE -> NAME -> =, (), ,;
 *
 * = -> ASSIGN -> STATEMENT -> ;
 *
 * STATEMENT -> (LITERAL, VARIABLE, FUNCTION CALL, OPERATORS)... -> ;
 *
 * () -> FUNCTION -> FUNCTION PARAMETER -> ) -> BLOCK (IS EQUAL TO {})
 *
 * ,; -> UNASSIGNED VARIABLE OR FUNCTION PARAMETER
 *
 */

/*
node *generate_ast(node *nodes) {
}
*/

/*
 * How do I generate the AST? I can't just add every declaration to the global
 * space because scope exists. I feel that there's a recursive way to take a
 * block of tokens, turn it into a plausible tree, and then add it to the
 * greater bit of the abstract syntax tree.
 */

// Prototype because it complains about multiple function definitions (It's used
// before it's defined).
node *parse_precedence(precedence precedence, token **token_pointer);

node *parse_number(token **token_pointer) {
  node *number_node = generate_node(NODE_LITERAL);
  // Get value from that token that sent us here
  int value = atoi(pop_token(token_pointer)->value);
  // The data pointer of all nodes is vector so put the value inside the vector
  vector_add(&number_node->data, value);

  return number_node;
}

node *parse_unary(token **token_pointer) {}


node *parse_binary(token **token_pointer) {
  // We need to somehow take the last expression of the parser, the next
  // expression and the math symbol and combine them.
  //
  // Kinda like int i = (EXPRESSION) <- grab value + parse this one ->
  // (EXPRESSION)
}
/*
 * Tutorial code:
static void binary() {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default: return; // Unreachable.
  }
}
 */
node *parse_grouping(token **token_pointer) {
  expect_token_value(TOKEN_PUNCTUATION, '(', token_pointer);
  node *ast = parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
  expect_token_value(TOKEN_PUNCTUATION, ')', token_pointer);
  return ast;
}

node *parse_precedence(precedence precedence, token **token_pointer) {
  token *current_token = peek_token(token_pointer);
  node *ast = generate_node(NODE_NONE);
  node *current_expression;
  node *last_expression;

  // Keep parsing equal or higher precedence values, or ';' is found.
  // Example: 10 * !(false + false)
  // Parses 10 as number then parses (false + false) then ! then *
  while (precedence <= get_precedence(current_token->type)) {
    switch (current_token->type) {
    case TOKEN_PUNCTUATION:
      if (current_token->value[0] == ';') {
        return ast;
      } else if (current_token->value[0] == '(') {
        current_expression = parse_grouping(token_pointer);
      }

    case TOKEN_INT:
      current_expression = parse_number(token_pointer);

    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
      current_expression = parse_binary(last_expression, token_pointer);

    default:
      // TODO: Better error handling
      printf("Error when parsing precedence: Unknown token %i", current_token->type);
      return ast;
    }
    last_expression = current_expression;
    vector_add(&ast.next_nodes, current_expression);
  }
}

// Wrapper for parse precedence
node *parse_expression(token **token_pointer) {
  expect_token(TOKEN_EQUALS, token_pointer);
  return parse_precedence(PRECEDENCE_ASSIGNMENT, token_pointer);
}

node parse_declaration(token **token_pointer) {
  expect_token(TOKEN_EQUALS, token_pointer);
  parse_expression(token_pointer);
}

node parse_function(token **token_pointer) {}

// Parse a type. Such as variable or function declaration.
node *parse_type(token **token_pointer) {
  /*
   * The type of the declaration (like variable, function, parameter) requires
   * going through type, pointer amount, then name.
   */
  node *ast;

  token *type = expect_token(TOKEN_TYPE, token_pointer);

  // Super messy. Calculates "*" amount
  int pointer_amount = 0;
  while (true) {
    if (pop_token(token_pointer)->type != TOKEN_STAR) {
      backtrack_token(token_pointer);
      break;
    }
    pointer_amount++;
  }

  token *name = expect_token(TOKEN_NAME, token_pointer);

  token *current_token = peek_token(token_pointer);
  if (current_token->type == TOKEN_EQUALS) {
    parse_declaration(token_pointer);
  } else if (current_token->type == TOKEN_PUNCTUATION) {
    if (current_token->value[0] == '(') {
      parse_function(token_pointer);
    } else if (current_token->value[0] == ';') {
    }
  }
}

// Parses a block of C tokens and outputs the block's AST reprisentation.
node *parse_block(token **token_pointer) {
  // End of token stream? NULL.
  if (is_at_end(token_pointer)) {
    return NULL;
  }

  token *current_token = peek_token(token_pointer);
  node *ast = generate_node(NODE_END);

  switch (current_token->type) {
  case TOKEN_END:
    return ast;
  case TOKEN_TYPE:
    parse_type(token_pointer);
    break;
  }
}

// Questions for code style:
//
// What to return when parsing error? Do I exit?
//
// Do I pop the token, then give it to the function directly, along with the
// current_token pointer?
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
// Takes in tokens, outputs an Abstract Syntax Tree (AST)
node *parser(token *tokens) {
  node *ast = generate_node(NODE_START);
  // Pointer to pointer so we can store state of which token we're on
  token **token_pointer = &tokens;

  for (int i = 0; i < vector_size(tokens); i++) {
  }

  return ast;
}
