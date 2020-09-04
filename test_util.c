#include "test_util.h"

#include <stdio.h>

void print_pass(const char *message) {
    printf(" - \e[1;32mPASS\e[1;0m %s\n", message);
}

void print_fail(const char *message) {
    printf(" - \e[1;31mFAIL\e[1;0m %s\n", message);
}