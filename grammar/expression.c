#include "grammar.h"

#include <stdlib.h>
#include <stdio.h>

#define ReturnUnexpectedCharacters(output) \
    output.status = TRY_ERROR; \
    output.error.location = working; \
    output.error.desc = "Unexpected characters"; \
    return output;

TryExpression_t parse_left_expression(const ConstString_t str) {
    TryExpression_t output;
    ConstString_t working = strip_whitespace(str);
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
    TryVariable_t var = parse_variable(str);
    if (var.status == TRY_SUCCESS) {
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_DECLARATION;
        output.value.decl = malloc(sizeof(Variable_t));
        *output.value.decl = var.value;
        return output;
    }
    TryConstString_t identifier = find_identifier(working);
    if (identifier.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, identifier.value).value);
        if (working.begin != working.end) {
            ReturnUnexpectedCharacters(output);
        }
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_IDENTIFIER;
        output.value.identifier = new_alloc_const_string_from_const_str(identifier.value);
        return output;
    }
    GrammarPropagateError(var, output);
}

TryExpression_t parse_right_expression(const ConstString_t str) {
    TryExpression_t output;
    ConstString_t working = strip_whitespace(str);
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
            ReturnUnexpectedCharacters(output);
        }
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_UINT_LIT;
        output.value.uint_lit = integer.value.integer;
        return output;
    }
    TryConstString_t str_lit = find_string_lit(working, '"', '\\');
    if (str_lit.status == TRY_ERROR) {
        GrammarPropagateError(str_lit, output);
    }
    else if (str_lit.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, str_lit.value).value);
        if (working.begin != working.end) {
            ReturnUnexpectedCharacters(output);
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
    if (char_lit.status == TRY_ERROR) {
        GrammarPropagateError(char_lit, output);
    }
    else if (char_lit.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, char_lit.value).value);
        if (working.begin != working.end) {
            ReturnUnexpectedCharacters(output);
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
    if (identifier.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, identifier.value).value);
        if (working.begin != working.end) {
            ReturnUnexpectedCharacters(output);
        }
        output.status = TRY_SUCCESS;
        output.value.variant = EXPRESSION_IDENTIFIER;
        output.value.identifier = new_alloc_const_string_from_const_str(identifier.value);
        return output;
    }
    output.status = TRY_NONE;
    return output;
}

size_t print_expression(char *buffer, const Expression_t *const expr) {
    buffer[0] = 0;
    switch (expr->variant) {
        case EXPRESSION_OPERATOR:
            return print_operator(buffer, expr->operator);
        case EXPRESSION_IDENTIFIER:
            return sprintf(buffer, "%.*s",
                (int)(expr->identifier.end - expr->identifier.begin), expr->identifier.begin);
        case EXPRESSION_TYPE:
            return print_type(buffer, expr->type);
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