#include "tests.h"
#include "../grammar.h"
#include "../../test_util.h"

#include <stdio.h>

const static Case_t cases[] = {
    {false, "", NULL},
    {false, " ", NULL},
    {true,  "void", NULL},
    {true,  "char", NULL},
    {true,  "signed char", "char"},
    {true,  "unsigned char", NULL},
    {true,  "short", NULL},
    {true,  "signed short", "short"},
    {true,  "unsigned short", NULL},
    {true,  "int", NULL},
    {true,  "signed int", "int"},
    {true,  "unsigned int", NULL},
    {true,  "long", NULL},
    {true,  "signed long", "long"},
    {true,  "unsigned long", NULL},
    {true,  "long long", NULL},
    {true,  "signed long long", "long long"},
    {true,  "unsigned long long", NULL},
    {true,  "float", NULL},
    {true,  "double", NULL},
    {false, "struct int", NULL},
    {false, "struct enum", NULL},
    {false, "struct register", NULL},
    {true,  "struct Thing_t", NULL},
    {true,  "union Thing_t", NULL},
    {true,  "enum Thing_t", NULL},
    {true,  "struct Thing_t { }", NULL},
    {true,  "struct { }", NULL},
    {true,  "struct Thing { struct Thing *next; }", NULL},
    {true,  "struct { int x; }", NULL},
    {false, "struct { wrong }", NULL},
    {true,  "struct{int x;}", "struct { int x; }"},
    {true,  "struct { int x ; }", "struct { int x; }"},
    {true,  "struct { struct Datatype { int x; int y; }; Datatype data; }", NULL},
    {true,  "struct { struct { }; }", NULL},
    {true,  "struct { struct { struct { struct { }; }; }; }", NULL},
    {true,  "struct { struct { int x; int y; }; char *str; }", NULL},
    {true,  "enum { RED, YELLOW, GREEN }", "enum { RED = 0, YELLOW = 1, GREEN = 2 }"},
    {true,  "enum { RED = 1, YELLOW = 5, GREEN = 1892234 }", NULL},
    {true,  "enum { RED = 5, YELLOW, GREEN }", "enum { RED = 5, YELLOW = 6, GREEN = 7 }"},
    {false, "enum { RED = }", NULL},
    {false, "enum { RED GREEN }", NULL},
    {false, "enum { struct }", NULL},
    {true,  "one", NULL},
    {false, NULL, NULL}
};

static TryCharPtr_t case_func(const ConstString_t str) {
    TryCharPtr_t output;
    static char buffer[0x10000];
    TryType_t type = parse_type(str);
    if (type.status == TRY_SUCCESS) {
        print_type(buffer, &type.value);
        output.status = TRY_SUCCESS;
        output.value = buffer;
        return output;
    }
    else if (type.status == TRY_ERROR) {
        GrammarPropagateError(type, output);
    }
    else {
        output.status = TRY_NONE;
        return output;
    }
}

int test_types() {
    printf("Running test_types() ...\n");
    return test_fixture(cases, case_func, TEST_INPUT_STRING);
}