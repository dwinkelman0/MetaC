#include "tests.h"
#include "../../test_util.h"

#include <stdio.h>
#include <string.h>

int test_fixture(const Case_t *const cases, TryCharPtr_t (*const func)(const ConstString_t str)) {
    char print_buffer[0x10000];
    int n_tests = 0;
    int n_failed_tests = 0;

    const Case_t *c = cases;
    while (c->input) {
        ++n_tests;
        AConstString_t alloced_str = new_alloc_const_string_from_cstr(c->input);
        ConstString_t wrapped_str;
        wrapped_str.begin = alloced_str.begin;
        wrapped_str.end = alloced_str.end;
        TryCharPtr_t result = func(wrapped_str);
        if (result.status == TRY_SUCCESS) {
            if (c->succeeds) {
                const char *output = c->output ? c->output : c->input;
                if (!strcmp(result.value, output)) {
                    sprintf(print_buffer, "(expected success) %s",
                        c->input);
                    print_pass(print_buffer);
                }
                else {
                    sprintf(print_buffer, "(expected success, got \"%s\") %s",
                        result.value, c->input);
                    print_fail(print_buffer);
                    ++n_failed_tests;
                }
            }
            else {
                sprintf(print_buffer, "(expected failure, got \"%s\") %s",
                    result.value, c->input);
                print_fail(print_buffer);
                ++n_failed_tests;
            }
        }
        else {
            if (c->succeeds) {
                if (result.status == TRY_ERROR) {
                    sprintf(print_buffer, "(expected success, got \"%s\" at \"%.*s\") %s",
                        result.error.desc, (int)(result.error.location.end - result.error.location.begin), result.error.location.begin, c->input);
                    print_fail(print_buffer);
                    ++n_failed_tests;
                }
                else {
                    sprintf(print_buffer, "(expected success, got none) %s",
                        c->input);
                    print_fail(print_buffer);
                    ++n_failed_tests;
                }
            }
            else {
                if (result.status == TRY_ERROR) {
                    sprintf(print_buffer, "(expected failure, got \"%s\" at \"%.*s\") %s",
                        result.error.desc, (int)(result.error.location.end - result.error.location.begin), result.error.location.begin, c->input);
                    print_pass(print_buffer);
                }
                else {
                    sprintf(print_buffer, "(expected failure, got none) %s",
                        c->input);
                    print_pass(print_buffer);
                }
            }
        }
        free_alloc_const_string(&alloced_str);
        ++c;
    }
    printf("%d failures out of %d tests\n", n_failed_tests, n_tests);
    return n_failed_tests;
}