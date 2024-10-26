#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "_tests.h"

completion_type assert(char *string1, char *string2) {
  if (strcmp(string1, string2) == 0) {
    return PASSED;
  } else {
    return FAILED;
  }
}

completion_type test_lexer() {
  // Read in file, pass it into lexer, check output with some hard-coded tokens.
  
  return PASSED;
}
