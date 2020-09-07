#include "tests.h"
#include "../grammar.h"
#include "../../test_util.h"

#include <stdio.h>

const static Case_t cases[] = {
    {true,  "tests/grammar/scope/basic0.in", NULL},
    {false, "tests/grammar/scope/basic1-bad_braces.in", NULL},
    {false, "tests/grammar/scope/basic2-bad_quotes.in", NULL},
    {true,  "tests/grammar/scope/basic3-bad_op.in", "tests/grammar/scope/basic3-bad_op.out"},
    {true,  "tests/grammar/scope/basic4-empty.in", NULL},
    {true,  "tests/grammar/scope/basic5-struct.in", NULL},
    {true,  "tests/grammar/scope/nested0.in", NULL},
    {true,  "tests/grammar/scope/nested1-recursion.in", NULL},
    {false, NULL, NULL}
};

static TryCharPtr_t case_func(const ConstString_t str) {
    TryCharPtr_t output;
    static char buffer[0x10000];
    ErrorLinkedListNode_t *errors = NULL;
    TryScope_t op = parse_scope(str, &errors);
    if (op.status == TRY_SUCCESS) {
        print_scope(buffer, &op.value, 0);
        output.status = TRY_SUCCESS;
        output.value = buffer;
        return output;
    }
    else if (op.status == TRY_ERROR) {
        GrammarPropagateError(op, output);
    }
    else {
        output.status = TRY_NONE;
        return output;
    }
}

int test_scope() {
    printf("Running test_scope() ...\n");
    return test_fixture(cases, case_func, TEST_INPUT_FILE);
}