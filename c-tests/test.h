#ifndef test_h
#define test_h

#include <stdio.h>

typedef enum {
  FAILED,
  PASSED,
} completion_type;

#define assert(condition)                                                      \
  ({                                                                           \
    if ((condition) == FAILED)                                                 \
      printf("Assertion failed: %s, %s:%s():%d:\n", #condition, __FILE__,      \
             __FUNCTION__, __LINE__);                                          \
    condition;                                                                 \
  })

#define error(message, ...)                                                    \
  ({                                                                           \
    fprintf(stderr, "Error: " message "\n%s:%s():%d:\n\n", ##__VA_ARGS__,      \
            __FILE__, __FUNCTION__, __LINE__);                                 \
    exit(1);                                                                   \
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

#endif
