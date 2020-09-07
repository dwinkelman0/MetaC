#ifndef _GRAMMAR_GRAMMAR_H_
#define _GRAMMAR_GRAMMAR_H_

#include "util.h"

#include <stddef.h>

TryType_t parse_type(const ConstString_t str);
TryVariable_t parse_variable(const ConstString_t str);
TryExpression_t parse_left_expression(const ConstString_t str);
TryExpression_t parse_right_expression(const ConstString_t str);
TryExpression_t parse_type_expression(const ConstString_t str);

/**
 * Parse an operator
 *  SUCCESS: an operator was parsed
 *  NONE: no possible operator was found
 */
TryOperator_t parse_operator(const ConstString_t str);

/**
 * Parse a scope into a linked list of statements; the first and last
 * characters must be braces
 *  SUCCESS: a scope was parsed
 *  ERROR: there was a syntactic problem with the scope itself (i.e. contained
 *         scopes/controls have syntactic problem, expressions not terminated
 *         in semicolons)
 */
TryScope_t parse_scope(const ConstString_t str, ErrorLinkedListNode_t **const errors);

size_t print_type(char *buffer, const Type_t *const type);
size_t print_variable(char *buffer, const Variable_t *const var);
size_t print_expression(char *buffer, const Expression_t *const expr, const Operator_t *const parent_op);
size_t print_operator(char *buffer, const Operator_t *const op);
size_t print_scope(char *buffer, const Scope_t *scope, const int32_t depth);
size_t print_statement(char *buffer, const Statement_t *stmt, const uint32_t depth);

uint32_t operator_precedence(const OperatorVariant_t variant);

#endif