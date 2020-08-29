#include "grammar.h"
#include "tests/variables.h"

#include <stdio.h>
#include <string.h>

int main() {
    size_t n_failures = test_variables();
    printf("%zu failures in test_variables\n", n_failures);
}