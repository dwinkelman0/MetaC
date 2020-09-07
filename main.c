#include "grammar/tests/tests.h"

#include <stdio.h>
#include <string.h>

int main() {
    size_t num_failures = 0;
    num_failures += test_types();
    num_failures += test_derived_types();
    num_failures += test_operator();
    num_failures += test_scope();
    printf("\e[1;38;5;207m%zu failures\e[1;0m\n", num_failures);
    return 0;
}