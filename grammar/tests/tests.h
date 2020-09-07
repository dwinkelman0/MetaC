#ifndef _GRAMMAR_TESTS_TESTS_H_
#define _GRAMMAR_TESTS_TESTS_H_

#include <stdbool.h>

#include "../grammar.h"

typedef struct {
    bool succeeds;
    const char *input;
    const char *output;
} Case_t;

typedef enum {
    TEST_INPUT_STRING,
    TEST_INPUT_FILE
} TestInputVariant_t;

int test_fixture(const Case_t *const cases, TryCharPtr_t (*const func)(const ConstString_t str), const TestInputVariant_t input_variant);

int test_types();
int test_derived_types();
int test_operator();
int test_scope();

#endif