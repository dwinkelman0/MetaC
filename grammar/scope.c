#include "grammar.h"

#include <stdio.h>
#include <stdlib.h>

TryVariable_t parse_typedef(const ConstString_t str) {
    TryVariable_t output;
    TryConstString_t typedef_str = find_string(str, const_string_from_cstr("typedef"));
    if (typedef_str.status == TRY_SUCCESS) {
        ConstString_t working = strip(str, typedef_str.value).value;
        TryConstString_t ws = find_whitespace(working);
        if (ws.status == TRY_NONE) {
            output.status = TRY_ERROR;
            output.error.location = working;
            output.error.desc = "There must be whitespace after a typedef";
            return output;
        }
        working = strip_whitespace(working);
        output = parse_variable(working);
        return output;
    }
    output.status = TRY_NONE;
    return output;
}

/**
 * Parse a statement
 *  SUCCESS: a statement was parsed
 *  NONE: a statement was not found
 *  ERROR: there was an error with statement syntax
 */
TryStatement_t parse_statement(const ConstString_t str, ErrorLinkedListNode_t ***const errors, ConstString_t *const stmt_str) {
    TryStatement_t output;
    output.value.variant = STATEMENT_DECLARATION;
    ConstString_t working = strip_whitespace(str);
    stmt_str->begin = str.begin;

    if (*working.begin == '{') {
        TryConstString_t braces = find_closing(working, '{', '}');
        GrammarPropagateError(braces, output);
        if (braces.status == TRY_SUCCESS) {
            stmt_str->end = braces.value.end;
            ConstString_t contents;
            contents.begin = braces.value.begin + 1;
            contents.end = braces.value.end - 1;
            TryScope_t scope = parse_scope(contents, errors);
            GrammarPropagateError(scope, output);
            if (scope.status == TRY_SUCCESS) {
                output.status = TRY_SUCCESS;
                output.value.str = working;
                output.value.variant = STATEMENT_SCOPE;
                output.value.scope = malloc(sizeof(Scope_t));
                *output.value.scope = scope.value;
                return output;
            }
        }
    }

    TryConstString_t keyword;
    Control_t control;
    if ((keyword = find_string(working, const_string_from_cstr("break"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_BREAK;
    }
    else if ((keyword = find_string(working, const_string_from_cstr("continue"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_CONTINUE;
    }
    else if ((keyword = find_string(working, const_string_from_cstr("return"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_RETURN;
    }
    if (output.value.variant == STATEMENT_CONTROL) {
        TryConstString_t semicolon = find_string_nesting_sensitive(working, const_string_from_cstr(";"));
        GrammarPropagateError(semicolon, output);
        if (semicolon.status == TRY_NONE) {
            output.status = TRY_ERROR;
            output.error.location = keyword.value;
            output.error.desc = "Control statement should be ended by a ';'";
            return output;
        }
        ConstString_t following;
        following.begin = keyword.value.end;
        following.end = semicolon.value.begin;
        TryConstString_t ws = find_whitespace(following);
        following = strip_whitespace(following);
        if (following.begin != following.end) {
            if (control.variant == CONTROL_RETURN) {
                TryExpression_t expr = parse_right_expression(following);
                GrammarPropagateError(expr, output);
                if (expr.status == TRY_NONE) {
                    output.status = TRY_ERROR;
                    output.error.location = following;
                    output.error.desc = "return accepts only right expressions";
                    return output;
                }
                control.ret = expr.value;
            }
            else {
                output.status = TRY_ERROR;
                output.error.location = following;
                output.error.desc = "break/continue does not accept data";
                return output;
            }
        }
        else if (control.variant == CONTROL_RETURN) {
            control.ret.variant = EXPRESSION_VOID;
        }
        output.status = TRY_SUCCESS;
        output.value.control = malloc(sizeof(Control_t));
        *output.value.control = control;
        output.value.str.begin = working.begin;
        output.value.str.end = semicolon.value.end;
        stmt_str->end = semicolon.value.end;
        return output;
    }

    if ((keyword = find_string(working, const_string_from_cstr("if"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_IF;
    }
    else if ((keyword = find_string(working, const_string_from_cstr("for"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_FOR;
    }
    else if ((keyword = find_string(working, const_string_from_cstr("while"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_WHILE;
    }
    else if ((keyword = find_string(working, const_string_from_cstr("do"))).status == TRY_SUCCESS) {
        output.value.variant = STATEMENT_CONTROL;
        control.variant = CONTROL_DO;
    }
    if (output.value.variant == STATEMENT_CONTROL) {
        ConstString_t cont_working = strip_whitespace(strip(working, keyword.value).value);
        if (control.variant == CONTROL_IF || control.variant == CONTROL_WHILE || control.variant == CONTROL_FOR) {
            TryConstString_t cond_str = find_closing(cont_working, '(', ')');
            GrammarPropagateError(cond_str, output);
            if (cond_str.status == TRY_NONE) {
                output.status = TRY_ERROR;
                output.error.location = cont_working;
                output.error.desc = "Control statement needs condition in ()";
                return output;
            }
            cont_working = strip_whitespace(strip(cont_working, cond_str.value).value);
            cond_str.value.begin += 1;
            cond_str.value.end -= 1;
            if (control.variant == CONTROL_IF || control.variant == CONTROL_WHILE) {
                TryExpression_t cond = parse_right_expression(cond_str.value);
                GrammarPropagateError(cond_str, output);
                if (cond.status == TRY_NONE) {
                    output.status = TRY_ERROR;
                    output.error.location = cond_str.value;
                    output.error.desc = "Control statement needs a condition in ()";
                    return output;
                }
                control.condition = cond.value;
            }
            else {
                ConstString_t init_str, check_str, inc_str;
                TryConstString_t semicolon1_str = find_string_nesting_sensitive(cond_str.value, const_string_from_cstr(";"));
                GrammarPropagateError(semicolon1_str, output);
                if (semicolon1_str.status == TRY_NONE) {
                    output.status = TRY_ERROR;
                    output.error.location = cond_str.value;
                    output.error.desc = "For statement needs a first ';'";
                    return output;
                }
                init_str.begin = cond_str.value.begin;
                init_str.end = semicolon1_str.value.begin;
                cond_str.value.begin = semicolon1_str.value.end;
                TryConstString_t semicolon2_str = find_string_nesting_sensitive(cond_str.value, const_string_from_cstr(";"));
                GrammarPropagateError(semicolon2_str, output);
                if (semicolon2_str.status == TRY_NONE) {
                    output.status = TRY_ERROR;
                    output.error.location = cond_str.value;
                    output.error.desc = "For statement needs a second ';'";
                    return output;
                }
                check_str.begin = semicolon1_str.value.end;
                check_str.end = semicolon2_str.value.begin;
                inc_str.begin = semicolon2_str.value.end;
                inc_str.end = cond_str.value.end;
                TryExpression_t init_expr = parse_right_expression(init_str);
                GrammarPropagateError(init_expr, output);
                if (init_expr.status == TRY_NONE) {
                    control.ctrl_for.init = NULL;
                }
                else {
                    control.ctrl_for.init = malloc(sizeof(Expression_t));
                    *control.ctrl_for.init = init_expr.value;
                }
                TryExpression_t cond_expr = parse_right_expression(check_str);
                GrammarPropagateError(cond_expr, output);
                if (cond_expr.status == TRY_NONE) {
                    control.condition.variant = EXPRESSION_UINT_LIT;
                    control.condition.uint_lit = 1;
                }
                else {
                    control.condition = cond_expr.value;
                }
                TryExpression_t inc_expr = parse_right_expression(inc_str);
                GrammarPropagateError(inc_expr, output);
                if (inc_expr.status == TRY_NONE) {
                    control.ctrl_for.increment = NULL;
                }
                else {
                    control.ctrl_for.increment = malloc(sizeof(Expression_t));
                    *control.ctrl_for.increment = inc_expr.value;
                }
            }
        }
        ConstString_t exec_str;
        TryStatement_t exec = parse_statement(cont_working, errors, &exec_str);
        GrammarPropagateError(exec, output);
        if (exec.status == TRY_NONE) {
            output.status = TRY_ERROR;
            output.error.location = cont_working;
            output.error.desc = "Control needs a scope";
            return output;
        }
        control.exec = exec.value;
        output.value.str.begin = working.begin;
        output.value.str.end = exec_str.end;
        if (control.variant == CONTROL_IF) {
            control.ctrl_if.continuation = NULL;
            ConstString_t continuation_str;
            continuation_str.begin = exec_str.end;
            continuation_str.end = working.end;
            continuation_str = strip_whitespace(continuation_str);
            TryConstString_t else_str = find_string(continuation_str, const_string_from_cstr("else"));
            if (else_str.status == TRY_SUCCESS) {
                continuation_str = strip(continuation_str, else_str.value).value;
                TryConstString_t id_str = find_identifier(continuation_str);
                if (id_str.status == TRY_NONE) {
                    ConstString_t scope_str;
                    TryStatement_t else_stmt = parse_statement(continuation_str, errors, &scope_str);
                    GrammarPropagateError(else_stmt, output);
                    if (else_stmt.status == TRY_NONE) {
                        output.status = TRY_ERROR;
                        output.error.location = cont_working;
                        output.error.desc = "Else needs a scope";
                        return output;
                    }
                    control.ctrl_if.continuation = malloc(sizeof(Statement_t));
                    *control.ctrl_if.continuation = else_stmt.value;
                    output.value.str.end = scope_str.end;
                }
            }
        }
        output.status = TRY_SUCCESS;
        output.value.control = malloc(sizeof(Control_t));
        *output.value.control = control;
        stmt_str->end = output.value.str.end;
        print_const_string(output.value.str, "output.value.str");
        return output;
    }

    TryConstString_t parens = find_string_nesting_sensitive(working, const_string_from_cstr("("));
    GrammarPropagateError(parens, output);
    if (parens.status == TRY_SUCCESS) {
        ConstString_t func_str;
        func_str.begin = parens.value.begin;
        func_str.end = working.end;
        parens = find_closing(func_str, '(', ')');
        if (parens.status == TRY_SUCCESS) {
            func_str = strip(func_str, parens.value).value;
            TryConstString_t braces = find_string_nesting_sensitive(working, const_string_from_cstr("{"));
            if (braces.status == TRY_SUCCESS) {
                func_str.begin = braces.value.begin;
                braces = find_closing(func_str, '{', '}');
                if (braces.status == TRY_SUCCESS) {
                    ConstString_t func_sig;
                    func_sig.begin = working.begin;
                    func_sig.end = braces.value.begin;
                    ConstString_t scope_str;
                    scope_str.begin = braces.value.begin + 1;
                    scope_str.end = braces.value.end - 1;

                    TryVariable_t signature = parse_variable(func_sig);
                    if (    signature.status == TRY_SUCCESS &&
                            signature.value.has_name &&
                            signature.value.type->variant == DERIVED_TYPE_FUNCTION) {
                        TryScope_t scope = parse_scope(scope_str, errors);
                        if (scope.status == TRY_SUCCESS) {
                            Function_t *function = malloc(sizeof(Function_t));
                            function->signature = signature.value;
                            function->scope = scope.value;
                            output.status = TRY_SUCCESS;
                            output.value.variant = STATEMENT_FUNCTION;
                            output.value.function = function;
                            stmt_str->end = braces.value.end;
                            output.value.str.begin = working.begin;
                            output.value.str.end = braces.value.end;
                            return output;
                        }
                    }
                }
            }
        }
    }

    TryConstString_t semicolon = find_string_nesting_sensitive(working, const_string_from_cstr(";"));
    GrammarPropagateError(semicolon, output);
    if (semicolon.status == TRY_NONE) {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "Expected a semicolon";
        return output;
    }
    stmt_str->end = semicolon.value.end;
    ConstString_t op_str;
    op_str.begin = working.begin;
    op_str.end = semicolon.value.begin;
    TryOperator_t op = parse_operator(op_str);
    if (op.status == TRY_SUCCESS) {
        output.status = TRY_SUCCESS;
        output.value.str.begin = op_str.begin;
        output.value.str.end = semicolon.value.end;
        output.value.variant = STATEMENT_OPERATOR;
        output.value.operator = malloc(sizeof(Operator_t));
        *output.value.operator = op.value;
        return output;
    }
    else {
        TryVariable_t var = parse_variable(op_str);
        output.value.variant = STATEMENT_DECLARATION;
        if (var.status != TRY_SUCCESS) {
            var = parse_typedef(op_str);
            output.value.variant = STATEMENT_TYPEDEF;
        }
        if (var.status == TRY_SUCCESS && var.value.has_name) {
            output.status = TRY_SUCCESS;
            output.value.str.begin = op_str.begin;
            output.value.str.end = semicolon.value.end;
            output.value.declaration = malloc(sizeof(Variable_t));
            *output.value.declaration = var.value;
            return output;
        }
        else {
            **errors = malloc(sizeof(ErrorLinkedListNode_t));
            (**errors)->next = NULL;
            (**errors)->value.location = op_str;
            if (var.status == TRY_ERROR) {
                (**errors)->value.desc = var.error.desc;
            }
            else {
                (**errors)->value.desc = var.status != TRY_SUCCESS ?
                    "Operator, declaration, or typedef did not parse" :
                    "Declaration or typedef has no name";
            }
            *errors = &(**errors)->next;
        }
    }

    output.status = TRY_NONE;
    return output;
}

TryScope_t parse_scope(const ConstString_t str, ErrorLinkedListNode_t ***const errors) {
    TryScope_t output;
    if (str.begin == str.end) {
        output.status = TRY_NONE;
        return output;
    }
    output.status = TRY_SUCCESS;
    output.value.statements = NULL;
    StatementLinkedListNode_t **stmt_head = &output.value.statements;
    ConstString_t working = str;
    working = strip_whitespace(working);
    while (working.begin < working.end) {
        ConstString_t stmt_str;
        TryStatement_t stmt = parse_statement(working, errors, &stmt_str);
        GrammarPropagateError(stmt, output);
        working = strip_whitespace(strip(working, stmt_str).value);
        if (stmt.status == TRY_SUCCESS) {
            *stmt_head = malloc(sizeof(StatementLinkedListNode_t));
            (*stmt_head)->next = NULL;
            (*stmt_head)->value = stmt.value;
            stmt_head = &(*stmt_head)->next;
        }
    }
    output.status = TRY_SUCCESS;
    return output;
}

size_t print_scope(char *buffer, const Scope_t *const scope, const int32_t depth) {
    buffer[0] = 0;
    size_t num_chars = 0;
    const StatementLinkedListNode_t *statement = scope->statements;
    if (statement) {
        if (depth >= 0) {
            num_chars += sprintf(buffer, "{\n");
        }
        while (statement) {
            num_chars += sprintf(buffer + num_chars, "%*s", (depth + 1) * 4, "");
            num_chars += print_statement(buffer + num_chars, &statement->value, depth + 1);
            num_chars += sprintf(buffer + num_chars, "\n");
            statement = statement->next;
        }
        if (depth >= 0) {
            num_chars += sprintf(buffer + num_chars, "%*s}", 4 * depth, "");
        }
    }
    else {
        if (depth >= 0) {
            num_chars += sprintf(buffer, "{ }");
        }
    }
    return num_chars;
}

size_t print_statement(char *buffer, const Statement_t *const stmt, const int32_t depth) {
    buffer[0] = 0;
    size_t num_chars = 0;
    switch (stmt->variant) {
        case STATEMENT_OPERATOR:
            num_chars += print_operator(buffer + num_chars, stmt->operator);
            num_chars += sprintf(buffer + num_chars, ";");
            break;
        case STATEMENT_SCOPE:
            num_chars += print_scope(buffer + num_chars, stmt->scope, depth);
            break;
        case STATEMENT_DECLARATION:
            num_chars += print_variable(buffer + num_chars, stmt->declaration);
            num_chars += sprintf(buffer + num_chars, ";");
            break;
        case STATEMENT_CONTROL:
            switch (stmt->control->variant) {
                case CONTROL_IF:
                    num_chars += sprintf(buffer + num_chars, "if (");
                    num_chars += print_expression(buffer + num_chars, &stmt->control->condition, NULL);
                    num_chars += sprintf(buffer + num_chars, ") ");
                    num_chars += print_statement(buffer + num_chars, &stmt->control->exec, depth);
                    if (stmt->control->ctrl_if.continuation) {
                        num_chars += sprintf(buffer + num_chars, "\n%*selse ", depth * 4, "");
                        num_chars += print_statement(buffer + num_chars, stmt->control->ctrl_if.continuation, depth);
                    }
                    break;
                case CONTROL_WHILE:
                    num_chars += sprintf(buffer + num_chars, "while (");
                    num_chars += print_expression(buffer + num_chars, &stmt->control->condition, NULL);
                    num_chars += sprintf(buffer + num_chars, ") ");
                    num_chars += print_statement(buffer + num_chars, &stmt->control->exec, depth);
                    break;
                case CONTROL_FOR:
                    num_chars += sprintf(buffer + num_chars, "for (");
                    num_chars += print_expression(buffer + num_chars, stmt->control->ctrl_for.init, NULL);
                    num_chars += sprintf(buffer + num_chars, "; ");
                    num_chars += print_expression(buffer + num_chars, &stmt->control->condition, NULL);
                    num_chars += sprintf(buffer + num_chars, "; ");
                    num_chars += print_expression(buffer + num_chars, stmt->control->ctrl_for.increment, NULL);
                    num_chars += sprintf(buffer + num_chars, ") ");
                    num_chars += print_statement(buffer + num_chars, &stmt->control->exec, depth);
                    break;
                case CONTROL_BREAK:
                    num_chars += sprintf(buffer + num_chars, "break;");
                    break;
                case CONTROL_CONTINUE:
                    num_chars += sprintf(buffer + num_chars, "continue;");
                    break;
                case CONTROL_RETURN:
                    if (stmt->control->ret.variant == EXPRESSION_VOID) {
                        num_chars += sprintf(buffer + num_chars, "return;");
                    }
                    else {
                        num_chars += sprintf(buffer + num_chars, "return ");
                        num_chars += print_expression(buffer + num_chars, &stmt->control->ret, NULL);
                        num_chars += sprintf(buffer + num_chars, ";");
                    }
                    break;
            }
            break;
        case STATEMENT_TYPEDEF:
            num_chars += sprintf(buffer + num_chars, "typedef ");
            num_chars += print_variable(buffer + num_chars, stmt->tdef);
            num_chars += sprintf(buffer + num_chars, ";");
            break;
        case STATEMENT_FUNCTION:
            num_chars += print_variable(buffer + num_chars, &stmt->function->signature);
            num_chars += sprintf(buffer + num_chars, " ");
            num_chars += print_scope(buffer + num_chars, &stmt->function->scope, depth);
            break;
    }
    return num_chars;
}