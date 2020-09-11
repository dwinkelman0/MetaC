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
    {true,  "tests/grammar/scope/basic6-typedef.in", NULL},
    {true,  "tests/grammar/scope/basic7-bad_typedef.in", "tests/grammar/scope/basic7-bad_typedef.out"},
    {true,  "tests/grammar/scope/basic8-global.in", NULL},
    {true,  "tests/grammar/scope/nested0.in", NULL},
    {true,  "tests/grammar/scope/nested1-recursion.in", NULL},
    {true,  "tests/grammar/scope/nested2-functions.in", NULL},
    {true,  "tests/grammar/scope/nested3-errors.in", "tests/grammar/scope/nested3-errors.out"},
    {true,  "tests/grammar/scope/cond0-if.in", NULL},
    {true,  "tests/grammar/scope/cond1-for.in", NULL},
    {true,  "tests/grammar/scope/cond2-while.in", NULL},
    {true,  "tests/grammar/scope/cond3-break.in", NULL},
    {true,  "tests/grammar/scope/cond4-continue.in", NULL},
    {false, "tests/grammar/scope/cond5-break_no_semicolon.in", NULL},
    {true,  "tests/grammar/scope/cond6-for_no_check.in", "tests/grammar/scope/cond6-for_no_check.out"},
    {true,  "tests/grammar/scope/cond7-for_no_init_inc.in", NULL},
    {false, "tests/grammar/scope/cond8-if_no_cond.in", NULL},
    {true,  "tests/grammar/scope/cond9-if_inline.in", NULL},
    {true,  "tests/grammar/scope/cond10-if_inline.in", NULL},
    {true,  "tests/grammar/scope/cond11-return.in", NULL},
    {true,  "tests/grammar/scope/cond12-return_value.in", NULL},
    {false, "tests/grammar/scope/cond13-break_value.in", NULL},
    {true,  "tests/grammar/scope/func0.in", NULL},
    {true,  "tests/grammar/scope/func1-return_func.in", NULL},
    {true,  "tests/grammar/scope/func2-func_error.in", "tests/grammar/scope/func2-func_error.out"},
    {false, NULL, NULL}
};

static TryCharPtr_t case_func(const ConstString_t str) {
    TryCharPtr_t output;
    static char buffer[0x10000];
    ErrorLinkedListNode_t *errors = NULL;
    ErrorLinkedListNode_t **errors_head = &errors;
    TryScope_t op = parse_scope(str, &errors_head);

    ErrorLinkedListNode_t *it = errors;
    while (it) {
        printf("\e[1;38;5;14m----Scope Error----\e[1;0m got \e[38;5;207m\"%s\"\e[1;0m at \e[38;5;3m\"%.*s\"\e[1;0m\n",
            it->value.desc, (int)(it->value.location.end - it->value.location.begin), it->value.location.begin);
        it = it->next;
    }

    if (op.status == TRY_SUCCESS) {
        print_scope(buffer, &op.value, -1);
        buffer[strlen(buffer) - 1] = 0;
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