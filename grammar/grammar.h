#ifndef _GRAMMAR_GRAMMAR_H_
#define _GRAMMAR_GRAMMAR_H_

#include "util.h"

#include <stddef.h>

TryType_t parse_type(const ConstString_t str);
TryVariable_t parse_variable(const ConstString_t str);
TryExpression_t parse_left_expression(const ConstString_t str);
TryExpression_t parse_right_expression(const ConstString_t str);
TryExpression_t parse_type_expression(const ConstString_t str);
TryOperator_t parse_operator(const ConstString_t str);

size_t print_type(char *buffer, const Type_t *const type);
size_t print_variable(char *buffer, const Variable_t *const var);
size_t print_expression(char *buffer, const Expression_t *const expr);
size_t print_operator(char *buffer, const Operator_t *const op);

#endif