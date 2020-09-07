#include "tests.h"
#include "../../test_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_fixture(const Case_t *const cases, TryCharPtr_t (*const func)(const ConstString_t str), const TestInputVariant_t variant) {
    char print_buffer[0x10000];
    int n_tests = 0;
    int n_failed_tests = 0;

    const Case_t *c = cases;
    while (c->input) {
        char *input = NULL;
        char *output = NULL;
        if (variant == TEST_INPUT_STRING) {
            input = (char *)c->input;
            output = (char *)(c->output ? c->output : c->input);
        }
        else {
            const size_t malloc_size = 0x10000;
            FILE *fp = fopen(c->input, "r");
            input = malloc(malloc_size);
            fread(input, 1, malloc_size, fp);
            fclose(fp);
            if (c->output) {
                fp = fopen(c->output, "r");
                output = malloc(malloc_size);
                fread(output, 1, malloc_size, fp);
                fclose(fp);
            }
            else {
                output = input;
            }
        }

        ConstString_t wrapped_str = const_string_from_cstr(input);
        TryCharPtr_t result = func(wrapped_str);
        if (result.status == TRY_SUCCESS) {
            if (c->succeeds) {
                if (!strcmp(result.value, output)) {
                    sprintf(print_buffer, "(expected success) %s",
                        input);
                    print_pass(print_buffer);
                }
                else {
                    sprintf(print_buffer, "(expected success, got \"\e[38;5;3m%s\e[1;0m\") %s",
                        result.value, input);
                    print_fail(print_buffer);
                    ++n_failed_tests;
                }
            }
            else {
                sprintf(print_buffer, "(expected failure, got \"\e[38;5;3m%s\e[1;0m\") %s",
                    result.value, input);
                print_fail(print_buffer);
                ++n_failed_tests;
            }
        }
        else {
            if (c->succeeds) {
                if (result.status == TRY_ERROR) {
                    sprintf(print_buffer, "(expected success, got \e[38;5;207m\"%s\"\e[1;0m at \"\e[38;5;3m%.*s\e[1;0m\") %s",
                        result.error.desc, (int)(result.error.location.end - result.error.location.begin), result.error.location.begin, input);
                    print_fail(print_buffer);
                    ++n_failed_tests;
                }
                else {
                    sprintf(print_buffer, "(expected success, got none) %s",
                        input);
                    print_fail(print_buffer);
                    ++n_failed_tests;
                }
            }
            else {
                if (result.status == TRY_ERROR) {
                    sprintf(print_buffer, "(expected failure, got \e[38;5;207m\"%s\"\e[1;0m at \"\e[38;5;3m%.*s\e[1;0m\") %s",
                        result.error.desc, (int)(result.error.location.end - result.error.location.begin), result.error.location.begin, input);
                    print_pass(print_buffer);
                }
                else {
                    sprintf(print_buffer, "(expected failure, got none) %s",
                        input);
                    print_pass(print_buffer);
                }
            }
        }
        ++n_tests;
        ++c;
    }
    printf("%d failures out of %d tests\n", n_failed_tests, n_tests);
    return n_failed_tests;
}