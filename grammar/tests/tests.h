#ifndef _GRAMMAR_TESTS_TESTS_H_
#define _GRAMMAR_TESTS_TESTS_H_

#include <stdbool.h>

#include "../grammar.h"

typedef struct {
    bool succeeds;
    const char *input;
    const char *output;
} Case_t;

int test_fixture(const Case_t *const cases, TryCharPtr_t (*const func)(const ConstString_t str));

int test_types();
int test_derived_types();

#endif