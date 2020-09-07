#include "grammar.h"

#include <stdio.h>

TryScope_t parse_scope(const ConstString_t str, ErrorLinkedListNode_t **const errors) {
    TryScope_t output;
    if (str.end - str.begin <= 2 || *str.begin != '{' || *(str.end - 1) != '}') {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "Not a scope";
        return output;
    }
    output.status = TRY_SUCCESS;
    output.value.statements = NULL;
    StatementLinkedListNode_t **stmt_head = &output.value.statements;
    ErrorLinkedListNode_t **error_head = errors;
    ConstString_t working;
    working.begin = str.begin + 1;
    working.end = str.end - 1;
    while (working.begin < working.end) {
        TryConstString_t semicolon = find_string_nesting_sensitive(working, const_string_from_cstr(";"));
        GrammarPropagateError(semicolon, output);
        if (semicolon.status == TRY_NONE) {
            output.status = TRY_ERROR;
            output.error.location = working;
            output.error.desc = "Expected a semicolon";
            return output;
        }
        ConstString_t op_str;
        op_str.begin = working.begin;
        op_str.end = semicolon.value.begin;
        TryOperator_t op = parse_operator(op_str);
        if (op.status == TRY_SUCCESS) {
            *stmt_head = malloc(sizeof(StatementLinkedListNode_t));
            (*stmt_head)->next = NULL;
            (*stmt_head)->value.variant = STATEMENT_OPERATOR;
            (*stmt_head)->value.operator = malloc(sizeof(Operator_t));
            *(*stmt_head)->value.operator = op.value;
            stmt_head = &(*stmt_head)->next;
        }
        else {
            *error_head = malloc(sizeof(ErrorLinkedListNode_t));
            (*error_head)->next = NULL;
            (*error_head)->value.location = op_str;
            (*error_head)->value.desc = "Operator did not parse";
            error_head = &(*error_head)->next;
        }
        working.begin = semicolon.value.end;
        working = strip_whitespace(working);
    }
    return output;
}

size_t print_scope(char *buffer, const Scope_t *const scope, const int32_t depth) {
    buffer[0] = 0;
    size_t num_chars = 0;
    const StatementLinkedListNode_t *statement = scope->statements;
    if (statement) {
        num_chars += sprintf(buffer, "{\n");
        while (statement) {
            num_chars += print_statement(buffer + num_chars, &statement->value, depth + 1);
            num_chars += sprintf(buffer + num_chars, ";\n");
            statement = statement->next;
        }
        num_chars += sprintf(buffer + num_chars, "%*s}", 4 * depth, "");
    }
    else {
        num_chars += sprintf(buffer, "{ }");
    }
    return num_chars;
}

size_t print_statement(char *buffer, const Statement_t *const stmt, const uint32_t depth) {
    buffer[0] = 0;
    size_t num_chars = 0;
    switch (stmt->variant) {
        case STATEMENT_OPERATOR:
            num_chars += sprintf(buffer, "%*s", depth * 4, "");
            num_chars += print_operator(buffer + num_chars, stmt->operator);
            break;
    }
    return num_chars;
}