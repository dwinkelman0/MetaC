#include "tests.h"
#include "../grammar.h"
#include "../../test_util.h"

#include <stdio.h>

const static Case_t cases[] = {
    {true,  "x = 1", NULL},
    {true,  "int x = 5", NULL},
    {true,  "const char *str = \"hello\"", NULL},
    {true,  "str = \"hello\"", NULL},
    {true,  "str = \"\\n\"", NULL},
    {true,  "my_char = 'c'", NULL},
    {true,  "my_char = '\\''", NULL},
    {true,  "int other = this", NULL},
    {true,  "other = this", NULL},
    //{false, "other = int x", NULL},
    //{false, "5 = 6", NULL},
    {false, NULL, NULL}
};

static TryCharPtr_t case_func(const ConstString_t str) {
    TryCharPtr_t output;
    static char buffer[0x10000];
    TryOperator_t op = parse_operator(str);
    if (op.status == TRY_SUCCESS) {
        print_operator(buffer, &op.value);
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

int test_operator() {
    printf("Running test_operator() ...\n");
    return test_fixture(cases, case_func);
}