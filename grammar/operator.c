#include "grammar.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

static TryOperator_t parse_unary_prefix_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    const OperatorSpec_t *spec = args;
    TryConstString_t prefix = find_string(str, const_string_from_cstr((const char *)spec->parse_args));
    if (prefix.status == TRY_SUCCESS) {
        ConstString_t unary_str;
        unary_str.begin = prefix.value.end;
        unary_str.end = str.end;
        TryExpression_t unary_expr = parse_right_expression(unary_str);
        GrammarPropagateError(unary_expr, output);
        if (unary_expr.status == TRY_SUCCESS) {
            output.status = TRY_SUCCESS;
            output.value.n_operands = 1;
            output.value.uop = malloc(sizeof(Expression_t));
            *output.value.uop = unary_expr.value;
            return output;
        }
    }
    output.status = TRY_NONE;
    return output;
}

static TryOperator_t parse_binary_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    const OperatorSpec_t *spec = args;
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
            TryExpression_t right_expr = spec->op2_type == LEFT_EXPR ?
                parse_left_expression(right_str) :
                parse_right_expression(right_str);
            if (right_expr.status == TRY_SUCCESS) {
                output.value.lop = malloc(sizeof(Expression_t));
                *output.value.lop = left_expr.value;
                output.value.rop = malloc(sizeof(Expression_t));
                *output.value.rop = right_expr.value;
                output.status = TRY_SUCCESS;
                output.value.n_operands = 2;
                return output;
            }
        }
    }
    output.status = TRY_NONE;
    return output;
}

static TryOperator_t parse_cast_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    TryConstString_t parens = find_closing(str, '(', ')');
    if (parens.status == TRY_SUCCESS) {
        ConstString_t type_str, right_str;
        type_str.begin = parens.value.begin + 1;
        type_str.end = parens.value.end - 1;
        right_str.begin = parens.value.end;
        right_str.end = str.end;
        TryExpression_t type_expr = parse_type_expression(type_str);
        if (type_expr.status == TRY_SUCCESS) {
            TryExpression_t right_expr = parse_right_expression(right_str);
            if (right_expr.status == TRY_SUCCESS) {
                output.value.lop = malloc(sizeof(Expression_t));
                *output.value.lop = type_expr.value;
                output.value.rop = malloc(sizeof(Expression_t));
                *output.value.rop = right_expr.value;
                output.status = TRY_SUCCESS;
                output.value.n_operands = 2;
                return output;
            }
        }
    }
    output.status = TRY_NONE;
    return output;
}

static TryOperator_t parse_cond_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    TryConstString_t question = find_string_nesting_sensitive(str, const_string_from_cstr("?"));
    if (question.status == TRY_SUCCESS) {
        ConstString_t pred_str, working;
        pred_str.begin = str.begin;
        pred_str.end = question.value.begin;
        working.begin = question.value.end;
        working.end = str.end;
        TryConstString_t colon = find_last_string_nesting_sensitive(working, const_string_from_cstr(":"));
        if (colon.status == TRY_SUCCESS) {
            ConstString_t true_str, false_str;
            true_str.begin = working.begin;
            true_str.end = colon.value.begin;
            false_str.begin = colon.value.end;
            false_str.end = working.end;
            TryExpression_t pred_expr = parse_right_expression(pred_str);
            if (pred_expr.status == TRY_SUCCESS) {
                TryExpression_t true_expr = parse_right_expression(true_str);
                if (true_expr.status == TRY_SUCCESS) {
                    TryExpression_t false_expr = parse_right_expression(false_str);
                    if (false_expr.status == TRY_SUCCESS) {
                        output.value.pop = malloc(sizeof(Expression_t));
                        *output.value.pop = pred_expr.value;
                        output.value.top = malloc(sizeof(Expression_t));
                        *output.value.top = true_expr.value;
                        output.value.fop = malloc(sizeof(Expression_t));
                        *output.value.fop = false_expr.value;
                        output.status = TRY_SUCCESS;
                        output.value.n_operands = 3;
                        return output;
                    }
                }
            }
        }
    }
    output.status = TRY_NONE;
    return output;
}

static TryOperator_t parse_sizeof_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    TryConstString_t keyword = find_string(str, const_string_from_cstr("sizeof"));
    if (keyword.status == TRY_SUCCESS) {
        ConstString_t working = strip_whitespace(strip(str, keyword.value).value);
        TryConstString_t parens = find_closing(working, '(', ')');
        if (parens.status == TRY_SUCCESS) {
            ConstString_t contents;
            contents.begin = parens.value.begin + 1;
            contents.end = parens.value.end - 1;
            TryExpression_t inner_expr = parse_right_expression(contents);
            if (inner_expr.status != TRY_SUCCESS) {
                inner_expr = parse_type_expression(contents);  
            }
            if (inner_expr.status == TRY_SUCCESS) {
                output.value.uop = malloc(sizeof(Expression_t));
                *output.value.uop = inner_expr.value;
                output.status = TRY_SUCCESS;
                output.value.n_operands = 1;
                return output;
            }
        }
    }
    output.status = TRY_NONE;
    return output;
}

static TryOperator_t parse_postfix_closure_operator(const ConstString_t str, const void *args) {
    TryOperator_t output;
    const OperatorSpec_t *spec = args;
    const char *pair = spec->parse_args;
    TryConstString_t closure = find_last_closure_nesting_sensitive(str, pair[0], pair[1]);
    if (closure.status == TRY_SUCCESS) {
        ConstString_t ws;
        ws.begin = closure.value.end;
        ws.end = str.end;
        ws = strip_whitespace(ws);
        if (ws.begin == ws.end) {
            ConstString_t left_str, right_str;
            left_str.begin = str.begin;
            left_str.end = closure.value.begin;
            right_str.begin = closure.value.begin + 1;
            right_str.end = closure.value.end - 1;
            TryExpression_t left_expr = parse_right_expression(left_str);
            if (left_expr.status == TRY_SUCCESS) {
                TryExpression_t right_expr = parse_right_expression(right_str);
                if (right_expr.status == TRY_SUCCESS || right_str.begin == right_str.end) {
                    output.value.lop = malloc(sizeof(Expression_t));
                    *output.value.lop = left_expr.value;
                    if (right_expr.status == TRY_SUCCESS) {
                        output.value.rop = malloc(sizeof(Expression_t));
                        *output.value.rop = right_expr.value;
                    }
                    else {
                        output.value.rop = NULL;
                    }
                    output.status = TRY_SUCCESS;
                    output.value.n_operands = 2;
                    return output;
                }
            }
        }
    }
    output.status = TRY_NONE;
    return output;
}

static size_t print_unary_prefix_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    const OperatorSpec_t *spec = args;
    num_chars += sprintf(buffer, "%s", (const char *)spec->print_args);
    num_chars += print_expression(buffer + num_chars, op->uop, op);
    return num_chars;
}

static size_t print_binary_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    const OperatorSpec_t *spec = args;
    num_chars += print_expression(buffer + num_chars, op->lop, op);
    num_chars += sprintf(buffer + num_chars, "%s", (const char *)spec->print_args);
    num_chars += print_expression(buffer + num_chars, op->rop, op);
    return num_chars;
}

static size_t print_cast_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    num_chars += sprintf(buffer, "(");
    num_chars += print_expression(buffer + num_chars, op->lop, op);
    num_chars += sprintf(buffer + num_chars, ")");
    num_chars += print_expression(buffer + num_chars, op->rop, op);
    return num_chars;
}

static size_t print_cond_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    num_chars += print_expression(buffer, op->pop, op);
    num_chars += sprintf(buffer + num_chars, " ? ");
    num_chars += print_expression(buffer + num_chars, op->top, op);
    num_chars += sprintf(buffer + num_chars, " : ");
    num_chars += print_expression(buffer + num_chars, op->fop, op);
    return num_chars;
}

static size_t print_sizeof_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    num_chars += sprintf(buffer, "sizeof(");
    if (op->uop && op->uop->variant == EXPRESSION_OPERATOR) {
        num_chars += print_operator(buffer + num_chars, op->uop->operator);
    }
    else {
        num_chars += print_expression(buffer + num_chars, op->uop, op);
    }
    num_chars += sprintf(buffer + num_chars, ")");
    return num_chars;
}

static size_t print_postfix_closure_operator(char *buffer, const Operator_t *const op, const void *args) {
    size_t num_chars = 0;
    const OperatorSpec_t *spec = args;
    const char *pair = spec->print_args;
    num_chars += print_expression(buffer + num_chars, op->lop, op);
    num_chars += sprintf(buffer + num_chars, "%c", pair[0]);
    if (op->rop && op->rop->variant == EXPRESSION_OPERATOR) {
        num_chars += print_operator(buffer + num_chars, op->rop->operator);
    }
    else {
        num_chars += print_expression(buffer + num_chars, op->rop, op);
    }
    num_chars += sprintf(buffer + num_chars, "%c", pair[1]);
    return num_chars;
}

static OperatorSpec_t operators[] = {
    {OP_COMMA,       2, 15,   ",",    ", ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_ASSIGN,      2, 14,   "=",   " = ",  LEFT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_ADD_ASSIGN,  2, 14,  "+=",  " += ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SUB_ASSIGN,  2, 14,  "-=",  " -= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_MUL_ASSIGN,  2, 14,  "*=",  " *= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_DIV_ASSIGN,  2, 14,  "/=",  " /= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_MOD_ASSIGN,  2, 14,  "%=",  " %= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SL_ASSIGN,   2, 14, "<<=", " <<= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SR_ASSIGN,   2, 14, ">>=", " >>= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_AND_ASSIGN,  2, 14,  "&=",  " &= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_XOR_ASSIGN,  2, 14,  "^=",  " ^= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_OR_ASSIGN,   2, 14,  "|=",  " |= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_COND,        3, 13,  NULL,    NULL, RIGHT_EXPR, RIGHT_EXPR, RIGHT_EXPR, parse_cond_operator, print_cond_operator},
    {OP_LOGICAL_OR,  2, 12,  "||",  " || ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LOGICAL_AND, 2, 11,  "&&",  " && ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_BITWISE_OR,  2, 10,   "|",   " | ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_BITWISE_XOR, 2,  9,   "^",   " ^ ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_BITWISE_AND, 2,  8,   "&",   " & ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_EQ,          2,  7,  "==",  " == ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_NE,          2,  7,  "!=",  " != ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_GT,          2,  6,   ">",   " > ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LT,          2,  6,   "<",   " < ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_GE,          2,  6,  ">=",  " >= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_LE,          2,  6,  "<=",  " <= ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SL,          2,  5,  "<<",  " << ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SR,          2,  5,  ">>",  " >> ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_ADD,         2,  4,   "+",   " + ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_SUB,         2,  4,   "-",   " - ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_MUL,         2,  3,   "*",   " * ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_DIV,         2,  3,   "/",   " / ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_MOD,         2,  3,   "%",   " % ", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_POS,         1,  2,   "+",     "+", RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_unary_prefix_operator, print_unary_prefix_operator},
    {OP_NEG,         1,  2,   "-",     "-", RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_unary_prefix_operator, print_unary_prefix_operator},
    {OP_LOGICAL_NOT, 1,  2,   "!",     "!", RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_unary_prefix_operator, print_unary_prefix_operator},
    {OP_BITWISE_NOT, 1,  2,   "~",     "~", RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_unary_prefix_operator, print_unary_prefix_operator},
    {OP_CAST,        1,  2,  NULL,    NULL,  TYPE_EXPR, RIGHT_EXPR,    NO_EXPR, parse_cast_operator, print_cast_operator},
    {OP_DEREFERENCE, 1,  1,   "*",     "*", RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_unary_prefix_operator, print_unary_prefix_operator},
    {OP_ADDRESS,     1,  1,   "&",     "&", RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_unary_prefix_operator, print_unary_prefix_operator},
    {OP_SIZEOF,      1,  1,  NULL,    NULL, RIGHT_EXPR,    NO_EXPR,    NO_EXPR, parse_sizeof_operator, print_sizeof_operator},
    {OP_CALL,        2,  0,  "()",    "()", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_postfix_closure_operator, print_postfix_closure_operator},
    {OP_SUBSCRIPT,   2,  0,  "[]",    "[]", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_postfix_closure_operator, print_postfix_closure_operator},
    {OP_MEM_ACCESS,  2,  0,   ".",     ".", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator},
    {OP_PTR_ACCESS,  2,  0,  "->",    "->", RIGHT_EXPR, RIGHT_EXPR,    NO_EXPR, parse_binary_operator, print_binary_operator}
};

TryOperator_t parse_operator(const ConstString_t str) {
    TryOperator_t output;
    ConstString_t working = strip_whitespace(str);
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }
    for (int i = 0; i < 43; ++i) {
        OperatorSpec_t *spec = &operators[i];
        if (!spec->parse_func) {
            continue;
        }
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
    if (!spec->print_func) {
        return 0;
    }
    return spec->print_func(buffer, op, spec);
}

uint32_t operator_precedence(const OperatorVariant_t variant) {
    const OperatorSpec_t *spec = &operators[variant];
    assert(spec->variant == variant);
    return spec->precedence;
}