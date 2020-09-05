#include "tests.h"
#include "../grammar.h"
#include "../../test_util.h"

#include <stdio.h>

const static Case_t cases[] = {
    {true,  "x, \"hello\"", NULL},
    {false, "int x, 5", NULL},
    {true,  "x = 1", NULL},
    {true,  "int x = 5", NULL},
    {true,  "const char *str = \"hello\"", NULL},
    {true,  "str = \"hello\"", NULL},
    {true,  "str = \"\\n\"", NULL},
    {true,  "my_char = 'c'", NULL},
    {true,  "my_char = '\\''", NULL},
    {true,  "int other = this", NULL},
    {true,  "other = this", NULL},
    {false, "other = int x", NULL},
    {false, "5 = 6", NULL},
    {true,  "int x = true ? 1 : 0", NULL},
    {true,  "int x = true ? false ? 2 : 1 : 0", NULL},
    {true,  "int x = true ? \"true: \" : \"false: \"", NULL},
    {true,  "bool x = y || z", NULL},
    {true,  "bool x = y && z || a && b", NULL},
    {true,  "uint64_t x = y | z", NULL},
    {true,  "uint64_t x = y ^ z", NULL},
    {true,  "uint64_t x = y & z", NULL},
    {true,  "bool x = y == z", NULL},
    {true,  "bool x = y != z", NULL},
    {true,  "bool x = y > z", NULL},
    {true,  "bool x = y < z", NULL},
    {true,  "bool x = y >= z", NULL},
    {true,  "bool x = y <= z", NULL},
    {true,  "uint64_t x = y << z", NULL},
    {true,  "uint64_t x = y >> z", NULL},
    {true,  "float x = y + z", NULL},
    {true,  "float x = y - z", NULL},
    {true,  "float x = y * z", NULL},
    {true,  "float x = y / z", NULL},
    {true,  "int x = y % z", NULL},
    {true,  "+time", NULL},
    {true,  "-time", NULL},
    {true,  "----time", NULL},
    {true,  "int x = y + -z", NULL},
    {true,  "int x = y - -z", NULL},
    {true,  "int x = y - +z", NULL},
    {true,  "int x = y + +z", NULL},
    {true,  "bool z = !t", NULL},
    {true,  "bool a = !!t && x", NULL},
    {true,  "uint64_t x = ~4", NULL},
    {true,  "(int)x", NULL},
    {true,  "(const DerivedType *)der", NULL},
    {true,  "(const void *)(const uint8_t *)bytes", NULL},
    {false, "(int x", NULL},
    {false, "(34567)thing", NULL},
    {true,  "char **str = &whitespace", NULL},
    {true,  "char str = *whitespace", NULL},
    {true,  "int x = a * *ptr", NULL},
    {true,  "int x = *(ptr + 2)", NULL},
    {true,  "size_t sz = sizeof(char)", NULL},
    {true,  "size_t sz = sizeof(char) * 128", NULL},
    {true,  "size_t sz = sizeof(\"Hello\")", NULL},
    {false, "size_t sz = sizeof(int x)", NULL},
    {true,  "size_t sz = sizeof(5 + 6)", NULL},
    {true,  "result = func()", NULL},
    {true,  "result = func(1, 2, 3)", NULL},
    {true,  "result = (*args->func)(1, 2, 3)", NULL},
    {true,  "item = things[3]", NULL},
    {true,  "item = things[i]", NULL},
    {true,  "item = things[i][j][k]", NULL},
    {true,  "x = obj.number", NULL},
    {true,  "x = obj->number", NULL},
    {true,  "x.number = 5 + 6", NULL},
    {true,  "x->number = 5 + 6", NULL},
    {true,  "(vec3 *const )pt.x = 6", NULL},
    {true,  "x.value.pointer->object.member->ref = \"Hello\"", NULL},
    {true,  "int x = (x + y) * z", NULL},
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