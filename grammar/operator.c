#include "grammar.h"

#include <stdlib.h>
#include <stdio.h>

typedef TryOperator_t (*ParseOperatorFunc_t)(const ConstString_t str, const void *args);
typedef size_t (*PrintOperatorFunc_t)(char *buffer, const Operator_t *const op, const void *args);
typedef enum {
    NO_EXPR,
    LEFT_EXPR,
    RIGHT_EXPR,
    TYPE_EXPR
} ExpressionTypeVariant_t;

typedef struct {
    OperatorVariant_t variant;
    uint32_t n_operands;
    uint32_t precedence;
    void *parse_args;
    void *print_args;
    ExpressionVariant_t op1_type;
    ExpressionVariant_t op2_type;
    ExpressionVariant_t op3_type;
    ParseOperatorFunc_t parse_func;
    PrintOperatorFunc_t print_func;
} OperatorSpec_t;

static TryOperator_t parse_binary_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    OperatorSpec_t *spec = args;
    TryConstString_t split = find_string_nesting_sensitive(str, const_string_from_cstr((const char *)spec->parse_args));
    if (split.status == TRY_SUCCESS) {
        ConstString_t left_str, right_str;
        left_str.begin = str.begin;
        left_str.end = split.value.begin;
        right_str.begin = split.value.end;
        right_str.end = str.end;
        TryExpression_t left_expr = spec->op1_type == LEFT_EXPR ?
            parse_left_expression(left_str) :
            parse_right_expression(left_str);
        if (left_expr.status == TRY_SUCCESS) {
            output.value.lop = malloc(sizeof(Expression_t));
            *output.value.lop = left_expr.value;
        }
        else {
            output.status = TRY_NONE;
            return output;
        }
        TryExpression_t right_expr = spec->op2_type == LEFT_EXPR ?
            parse_left_expression(right_str) :
            parse_right_expression(right_str);
        if (right_expr.status == TRY_SUCCESS) {
            output.value.rop = malloc(sizeof(Expression_t));
            *output.value.rop = right_expr.value;
        }
        else {
            output.status = TRY_NONE;
            return output;
        }
        output.status = TRY_SUCCESS;
        output.value.n_operands = 2;
        return output;
    }
    else {
        output.status = TRY_NONE;
        return output;
    }
}

static size_t print_binary_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    OperatorSpec_t *spec = args;
    num_chars += print_expression(buffer + num_chars, op->lop);
    num_chars += sprintf(buffer + num_chars, "%s", (const char *)spec->print_args);
    num_chars += print_expression(buffer + num_chars, op->rop);
    return num_chars;
}

static OperatorSpec_t operators[] = {
    {OP_COMMA,       2, 15,   ",",   ", ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_ASSIGN,      2, 14,   "=",  " = ",  LEFT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LOGICAL_OR,  2, 12,  "||", " || ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LOGICAL_AND, 2, 11,  "&&", " && ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_BITWISE_OR,  2, 10,   "|",  " | ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_BITWISE_XOR, 2,  9,   "^",  " ^ ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_BITWISE_AND, 2,  8,   "&",  " & ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_EQ,          2,  7,  "==", " == ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_NE,          2,  7,  "!=", " != ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_GT,          2,  6,   ">",  " > ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LT,          2,  6,   "<",  " < ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_GE,          2,  6,  ">=", " >= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LE,          2,  6,  "<=", " <= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SL,          2,  5,  "<<", " << ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SR,          2,  5,  ">>", " >> ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_ADD,         2,  4,   "+",  " + ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SUB,         2,  4,   "-",  " - ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_MUL,         2,  3,   "*",  " * ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_DIV,         2,  3,   "/",  " / ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_MOD,         2,  3,   "%",  " % ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
};

TryOperator_t parse_operator(const ConstString_t str) {
    TryOperator_t output;
    ConstString_t working = strip_whitespace(str);
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }
    for (int i = 0; i < 20; ++i) {
        OperatorSpec_t *spec = &operators[i];
        TryOperator_t op = spec->parse_func(working, spec);
        if (op.status == TRY_SUCCESS) {
            output.status = TRY_SUCCESS;
            output.value = op.value;
            output.value.variant = i;
            return output;
        }
    }
    output.status = TRY_NONE;
    return output;
}

size_t print_operator(char *buffer, const Operator_t *const op) {
    buffer[0] = 0;
    OperatorSpec_t *spec = &operators[op->variant];
    return spec->print_func(buffer, op, spec);
}