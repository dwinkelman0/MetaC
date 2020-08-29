#include "variables.h"
#include "../grammar.h"
#include "../util.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    bool succeeds;
    const char *input;
    const char *output;
} Case_t;

Case_t cases[] = {
    {true,  "int x", NULL},
    {true,  "const int x", NULL},
    {true,  "volatile long x", NULL},
    {true,  "int", NULL},
    {true,  "    int", "int"},
    {true,  "int    ", "int"},
    {true,  "int x  ", "int x"},
    {false, "64int thing", NULL},
    {false, "int 64thing", NULL},
    {false, "int$ wrong", NULL},
    {false, "int wrong#", NULL},
    {false, "int int thing", NULL},
    {true,  "void *x", NULL},
    {true,  "void *  x", "void *x"},
    {true,  "void*x", "void *x"},
    {true,  "void*", "void *"},
    {true,  "const char *const x", NULL},
    {true,  "const short *volatile x", NULL},
    {true,  "void ******x", NULL},
    {true,  "void **const **const *const weirdo", NULL},
    {true,  "void func()", NULL},
    {true,  "void func(int x)", NULL},
    {true,  "void func(int x, int y)", NULL},
    {true,  "void func(const char **str)", NULL},
    {true,  "void *func()", NULL},
    {true,  "void (*func)()", NULL},
    {true,  "void *(*func)()", NULL},
    {true,  "void *(*const func)()", NULL},
    {true,  "void *const (*const func)()", NULL},
    {true,  "char str[]", NULL},
    {true,  "char str[][]", NULL},
    {true,  "char str[6]", NULL},
    {true,  "char str[218]", NULL},
    {true,  "char str[0xda]", "char str[218]"},
    {true,  "char str[0xDA]", "char str[218]"},
    {true,  "char str  [4]", "char str[4]"},
    {true,  "char str[  4]", "char str[4]"},
    {true,  "char str[4  ]", "char str[4]"},
    {true,  "char str[4]  ", "char str[4]"},
    {true,  "char str[4][3][2][1]", NULL},
    {true,  "char **str[]", NULL},
    {true,  "char *(*str)[]", NULL},
    {true,  "char *(*str[6])[]", NULL},
    {true,  "void *(*(*func)[])()", NULL},
    {true,  "// comment\nint x", "int x"},
    {true,  "// comment\n\tint x", "int x"},
    {true,  "int // comment\nx", "int x"},
    {true,  "int x // comment", "int x"},
    {true,  "/* comment */ int x", "int x"},
    {true,  "int /* comment */ x", "int x"},
    {true,  "int/*comment*/x", "int x"},
    {true,  "int x /* comment */", "int x"},
    {false, NULL, NULL}
};

size_t test_variables() {
    size_t n_failures = 0;
    const Case_t *c = cases;
    while (c->input) {
        TryVariable_t var = parse_variable(c->input, strend(c->input));
        if (c->succeeds && !var.success) {
            printf(" - FAIL (expecting success, got failure \"%s\"): \"%s\"\n", var.error.desc, c->input);
            ++n_failures;
        }
        else if (!c->succeeds && var.success) {
            printf(" - FAIL (expecting failure, got success): \"%s\"\n", c->input);
            ++n_failures;
        }
        else if (!c->succeeds && !var.success) {
            printf(" - PASS (fails): \"%s\"\n", c->input);
        }
        else if (c->succeeds) {
            char buffer[0x1000];
            memset(buffer, 0, sizeof(buffer));
            print_variable(&var.value, buffer);
            if (strcmp(buffer, c->output ? c->output : c->input)) {
                printf(" - Failed (expecting \"%s\", got \"%s\"): \"%s\"\n", c->output ? c->output : c->input, buffer, c->input);
                ++n_failures;
            }
            else {
                printf(" - PASS (succeeds): \"%s\"\n", c->output ? c->output : c->input);
            }
        }
        ++c;
    }
    return n_failures;
}