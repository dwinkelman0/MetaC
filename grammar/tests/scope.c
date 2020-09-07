#include "tests.h"
#include "../grammar.h"
#include "../../test_util.h"

#include <stdio.h>

const static Case_t cases[] = {
    {true,  "tests/grammar/scope/basic0.in", NULL},
    {false, "tests/grammar/scope/basic1.in", NULL},
    {false, "tests/grammar/scope/basic2.in", NULL},
    {true,  "tests/grammar/scope/basic3.in", "tests/grammar/scope/basic3.out"},
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