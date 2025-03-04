#ifndef test_h
#define test_h

#include <stdio.h>

#define assert(condition)                                                      \
  ({                                                                           \
    if ((condition) == FAILED)                                                   \
      printf("Assertion failed: %s, in %s, line %d\n", #condition, __FILE__,        \
             __LINE__);                                                        \
    condition;                                                                 \
  })

// Run a test, print out fail condition, evaluate to either PASSED or FAILED
#define run_test(test)                                                         \
  ({                                                                           \
    completion_type result = test();                                           \
    if (result == PASSED)                                                      \
      printf("PASSED: %s\n", #test);                                           \
    else                                                                       \
      printf("FAILED: %s\n", #test);                                           \
    result;                                                                    \
  })

typedef enum {
  FAILED,
  PASSED,
} completion_type;

#endif
