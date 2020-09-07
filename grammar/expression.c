#include "grammar.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * If the string starts and ends with matching parentheses, extract the
 * interior string; returns the original string if no parentheses found
 */
ConstString_t strip_wrapping_parens(const ConstString_t str) {
    ConstString_t working = str;
    while (working.begin < working.end && working.begin[0] == '(') {
        TryConstString_t closing = find_closing(working, '(', ')');
        if (closing.status != TRY_SUCCESS) {
            break;
        }
        ConstString_t ws;
        ws.begin = closing.value.end;
        ws.end = working.end;
        ws = strip_whitespace(ws);
        if (ws.begin != ws.end) {
            break;
        }
        working.begin = closing.value.begin + 1;
        working.end = closing.value.end - 1;
    }
    return working;
}

TryExpression_t parse_left_expression(const ConstString_t str) {
    TryExpression_t output;
    ConstString_t working = strip_wrapping_parens(strip_whitespace(str));
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }
    TryVariable_t var = parse_variable(str);
    if (var.status == TRY_SUCCESS && var.value.has_name) {
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_DECLARATION;
        output.value.decl = malloc(sizeof(Variable_t));
        *output.value.decl = var.value;
        return output;
    }
    TryOperator_t op = parse_operator(working);
    if (op.status == TRY_SUCCESS) {
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_OPERATOR;
        output.value.operator = malloc(sizeof(Operator_t));
        *output.value.operator = op.value;
        return output;
    }
    TryConstString_t identifier = find_identifier(working);
    if (identifier.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, identifier.value).value);
        if (working.begin != working.end) {
            output.status = TRY_NONE;
            return output;
        }
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_IDENTIFIER;
        output.value.identifier = new_alloc_const_string_from_const_str(identifier.value);
        return output;
    }
    output.status = TRY_ERROR;
    output.error.location = str;
    output.error.desc = "Expected a left expression";
    return output;
}

TryExpression_t parse_right_expression(const ConstString_t str) {
    TryExpression_t output;
    ConstString_t working = strip_wrapping_parens(strip_whitespace(str));
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }
    TryOperator_t op = parse_operator(working);
    if (op.status == TRY_SUCCESS) {
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_OPERATOR;
        output.value.operator = malloc(sizeof(Operator_t));
        *output.value.operator = op.value;
        return output;
    }
    TryIntegerLiteral_t integer = find_integer(working);
    if (integer.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, integer.value.str).value);
        if (working.begin != working.end) {
            output.status = TRY_NONE;
            return output;
        }
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_UINT_LIT;
        output.value.uint_lit = integer.value.integer;
        return output;
    }
    TryConstString_t str_lit = find_string_lit(working, '"', '\\');
    GrammarPropagateError(str_lit, output);
    if (str_lit.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, str_lit.value).value);
        if (working.begin != working.end) {
            output.status = TRY_NONE;
            return output;
        }
        ConstString_t contents;
        contents.begin = str_lit.value.begin + 1;
        contents.end = str_lit.value.end - 1;
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_STR_LIT;
        output.value.str_lit = new_alloc_const_string_from_const_str(contents);
        return output;
    }
    TryConstString_t char_lit = find_string_lit(working, '\'', '\\');
    GrammarPropagateError(char_lit, output);
    if (char_lit.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, char_lit.value).value);
        if (working.begin != working.end) {
            output.status = TRY_NONE;
            return output;
        }
        ConstString_t contents;
        contents.begin = char_lit.value.begin + 1;
        contents.end = char_lit.value.end - 1;
        size_t len = contents.end - contents.begin;
        if ((len == 1 && contents.begin[0] != '\\') || (len == 2 && contents.begin[0] == '\\')) {
            output.status = TRY_SUCCESS;
            output.value.variant = EXPRESSION_CHAR_LIT;
            output.value.char_lit = new_alloc_const_string_from_const_str(contents);
            return output;
        }
        else {
            output.status = TRY_ERROR;
            output.error.location = contents;
            output.error.desc = "Character literal must contain 1 character";
            return output;
        }
    }
    TryConstString_t identifier = find_identifier(working);
    GrammarPropagateError(identifier, output);
    if (identifier.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, identifier.value).value);
        if (working.begin != working.end) {
            output.status = TRY_NONE;
            return output;
        }
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_IDENTIFIER;
        output.value.identifier = new_alloc_const_string_from_const_str(identifier.value);
        return output;
    }
    output.status = TRY_ERROR;
    output.error.location = str;
    output.error.desc = "Expected a right expression";
    return output;
}

TryExpression_t parse_type_expression(const ConstString_t str) {
    TryExpression_t output;
    ConstString_t working = strip_wrapping_parens(strip_whitespace(str));
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }
    TryVariable_t var = parse_variable(working);
    if (var.status == TRY_SUCCESS && !var.value.has_name) {
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_TYPE;
        output.value.type = malloc(sizeof(DerivedType_t));
        memcpy(output.value.type, var.value.type, sizeof(DerivedType_t));
        return output;
    }
    output.status = TRY_NONE;
    return output;
}

size_t print_expression(char *buffer, const Expression_t *const expr, const Operator_t *const parent_op) {
    if (!expr) {
        return 0;
    }
    buffer[0] = 0;
    Variable_t var;
    size_t num_chars = 0;
    switch (expr->variant) {
        case EXPRESSION_OPERATOR:
            if (parent_op && operator_precedence(parent_op->variant) < operator_precedence(expr->operator->variant)) {
                num_chars += sprintf(buffer, "(");
                num_chars += print_operator(buffer + num_chars, expr->operator);
                num_chars += sprintf(buffer + num_chars, ")");
                return num_chars;
            }
            else {
                return print_operator(buffer, expr->operator);
            }
        case EXPRESSION_IDENTIFIER:
            return sprintf(buffer, "%.*s",
                (int)(expr->identifier.end - expr->identifier.begin), expr->identifier.begin);
        case EXPRESSION_TYPE:
            var.type = expr->type;
            var.has_name = false;
            return print_variable(buffer, &var);
        case EXPRESSION_DECLARATION:
            return print_variable(buffer, expr->decl);
        case EXPRESSION_STR_LIT:
            return sprintf(buffer, "\"%.*s\"",
                (int)(expr->str_lit.end - expr->str_lit.begin), expr->str_lit.begin);
        case EXPRESSION_CHAR_LIT:
            return sprintf(buffer, "'%.*s'",
                (int)(expr->char_lit.end - expr->char_lit.begin), expr->char_lit.begin);
        case EXPRESSION_UINT_LIT:
            return sprintf(buffer, "%lu", expr->uint_lit);
    }
}