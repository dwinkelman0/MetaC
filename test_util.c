#include "test_util.h"

#include <stdio.h>

void print_pass(const char *message) {
    printf(" - \e[1;38;5;2mPASS\e[1;0m %s\n", message);
}

void print_fail(const char *message) {
    printf(" - \e[1;38;5;1mFAIL\e[1;0m %s\n", message);
}