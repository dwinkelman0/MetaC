#include "grammar.h"

#include <stdio.h>
#include <stdlib.h>

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
    working = strip_whitespace(working);
    while (working.begin < working.end) {

        if (*working.begin == '{') {
            TryConstString_t braces = find_closing(working, '{', '}');
            GrammarPropagateError(braces, output);
            if (braces.status == TRY_SUCCESS) {
                working.begin = braces.value.end;
                working = strip_whitespace(working);
                TryScope_t scope = parse_scope(braces.value, error_head);
                GrammarPropagateError(scope, output);
                if (scope.status == TRY_SUCCESS) {
                    *stmt_head = malloc(sizeof(StatementLinkedListNode_t));
                    (*stmt_head)->next = NULL;
                    (*stmt_head)->value.variant = STATEMENT_SCOPE;
                    (*stmt_head)->value.scope = malloc(sizeof(Scope_t));
                    *(*stmt_head)->value.scope = scope.value;
                    stmt_head = &(*stmt_head)->next;
                    while (*error_head) {
                        error_head = &(*error_head)->next;
                    }
                }
            }
            continue;
        }

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
            TryVariable_t var = parse_variable(op_str);
            if (var.status == TRY_SUCCESS && var.value.has_name) {
                *stmt_head = malloc(sizeof(StatementLinkedListNode_t));
                (*stmt_head)->next = NULL;
                (*stmt_head)->value.variant = STATEMENT_DECLARATION;
                (*stmt_head)->value.declaration = malloc(sizeof(Variable_t));
                *(*stmt_head)->value.declaration = var.value;
                stmt_head = &(*stmt_head)->next;
            }
            else {
                *error_head = malloc(sizeof(ErrorLinkedListNode_t));
                (*error_head)->next = NULL;
                (*error_head)->value.location = op_str;
                (*error_head)->value.desc = "Operator did not parse";
                error_head = &(*error_head)->next;
            }
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
            num_chars += sprintf(buffer + num_chars, ";\n");
            break;
        case STATEMENT_SCOPE:
            num_chars += sprintf(buffer, "%*s", depth * 4, "");
            num_chars += print_scope(buffer + num_chars, stmt->scope, depth);
            num_chars += sprintf(buffer + num_chars, "\n");
            break;
        case STATEMENT_DECLARATION:
            num_chars += sprintf(buffer, "%*s", depth * 4, "");
            num_chars += print_variable(buffer + num_chars, stmt->declaration);
            num_chars += sprintf(buffer + num_chars, ";\n");
            break;
    }
    return num_chars;
}