#include "tests.h"
#include "../grammar.h"
#include "../../test_util.h"

#include <stdio.h>
#include <string.h>

const static Case_t cases[] = {
    {false, "", NULL},
    {false, " ", NULL},
    {true,  "int x", NULL},
    {true,  "unsigned int x", NULL},
    {true,  "unsigned long long x", NULL},
    {true,  "Thing_t x", NULL},
    {true,  "const int x", NULL},
    {true,  "volatile long x", NULL},
    {true,  "const unsigned long long x", NULL},
    {true,  "conststr str", NULL},
    {true,  "volatileint int", NULL},
    {true,  "const conststr str", NULL},
    {true,  "int", NULL},
    {true,  "    int", "int"},
    {true,  "int    ", "int"},
    {true,  "int x  ", "int x"},
    {false, "(int x)", "int x"},
    {true,  "int (x)", "int x"},
    {false, "const (int x)", NULL},
    {false, "64int thing", NULL},
    {false, "int 64thing", NULL},
    {false, "int$ wrong", NULL},
    {false, "int wrong#", NULL},
    {false, "int int thing", NULL},
    {true,  "void *x", NULL},
    {true,  "void* x", "void *x"},
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
    {true,  "void (*func)(void (*callback)(int x), void *args)", NULL},
    {true,  "void (*func)(void (*callback)(int x, int y), void *args)", NULL},
    {true,  "void (*)()", NULL},
    {false, "void *()", NULL},
    {false, "void ()", NULL},
    {true,  "void(*)()", "void (*)()"},
    {false, "void func(", NULL},
    {false, "void func()(", NULL},
    {false, "void func())", NULL},
    {false, "void func)(", NULL},
    {false, "void func[)", NULL},
    {false, "void func(]", NULL},
    {false, "func(int x)", NULL},
    {true,  "char str[]", NULL},
    {true,  "char str[][]", NULL},
    {true,  "char []", NULL},
    {true,  "char str[6]", NULL},
    {true,  "char str[218]", NULL},
    {true,  "char str[0xda]", NULL},
    {true,  "char str[my_size * sizeof(int)]", NULL},
    {true,  "char str[sizes[inner[2]]]", NULL},
    {true,  "char str[4][3][2][1]", NULL},
    {true,  "char **str[]", NULL},
    {true,  "char *(*str)[]", NULL},
    {true,  "char *(*str[6])[]", NULL},
    {true,  "void *(*(*func)[])()", NULL},
    {false, "struct", NULL},
    {true,  "struct { }", NULL},
    {false, "struct {", NULL},
    {true,  "struct{}", "struct { }"},
    {true,  "struct Thing_t", NULL},
    {true,  "struct Thing_t { }", NULL},
    {true,  "struct Thing_t data", NULL},
    {true,  "struct Thing_t { } data", NULL},
    {true,  "struct { }", NULL},
    {true,  "struct { } data", NULL},
    {true,  "struct{int x;}data", "struct { int x; } data"},
    {true,  "const struct { const char *begin; const char *end; }", NULL},
    {true,  "double print(struct Pizza_t *pizza)", NULL},
    {true,  "double print(struct Pizza { double radius; } *pizza)", NULL},
    {false, NULL, NULL}
};

static TryCharPtr_t case_func(const ConstString_t str) {
    TryCharPtr_t output;
    static char buffer[0x10000];
    TryVariable_t var = parse_variable(str);
    if (var.status == TRY_SUCCESS) {
        print_variable(buffer, &var.value);
        output.status = TRY_SUCCESS;
        output.value = buffer;
        return output;
    }
    else if (var.status == TRY_ERROR) {
        GrammarPropagateError(var, output);
    }
    else {
        output.status = TRY_NONE;
        return output;
    }
}

int test_derived_types() {
    printf("Running test_derived_types() ...\n");
    return test_fixture(cases, case_func);
}