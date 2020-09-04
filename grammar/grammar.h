#ifndef _GRAMMAR_GRAMMAR_H_
#define _GRAMMAR_GRAMMAR_H_

#include "util.h"

#include <stddef.h>

TryType_t parse_type(const ConstString_t str);
TryVariable_t parse_variable(const ConstString_t str);

size_t print_type(char *buffer, const Type_t *const type);
size_t print_variable(char *buffer, const Variable_t *const var);

#endif