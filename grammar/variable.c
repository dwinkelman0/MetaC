#include "grammar.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const static char *qualifier_strs[] = {
    "",
    "const ",
    "volatile "
};

typedef struct {
    QualifierVariant_t variant;
    ConstString_t str;
} QualifierString_t;
typedef GrammarTryType(QualifierString_t) TryQualifierString_t;

/**
 * Check whether there is a qualifier at the beginning of the string; if so,
 * determine which one and return the string location
 *  SUCCESS: a qualifier was found
 *  NONE: a qualifier was not found
 */
TryQualifierString_t find_qualifier(const ConstString_t str) {
    TryQualifierString_t output;
    output.status = TRY_NONE;
    ConstString_t working = str;
    TryConstString_t qual_str;
    if ((qual_str = find_string(working, const_string_from_cstr("const"))).status == TRY_SUCCESS) {
        output.status = TRY_SUCCESS;
        output.value.variant = QUALIFIER_CONST;
    }
    else if ((qual_str = find_string(working, const_string_from_cstr("volatile"))).status == TRY_SUCCESS) {
        output.status = TRY_SUCCESS;
        output.value.variant = QUALIFIER_VOLATILE;
    }
    if (output.status == TRY_SUCCESS) {
        output.value.str = qual_str.value;
        working = strip(working, qual_str.value).value;
        TryConstString_t id_str = find_identifier(working);
        if (id_str.status == TRY_SUCCESS) {
            output.status = TRY_NONE;
        }
    }
    return output;
}

TryVariable_t parse_variable(const ConstString_t str) {
    TryVariable_t output;
    output.value.has_name = false;

    ConstString_t working = strip_whitespace(str);
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }

    DerivedType_t terminal;
    terminal.variant = DERIVED_TYPE_TERMINAL;
    terminal.terminal.qualifier = QUALIFIER_NONE;
    TryQualifierString_t qual_str = find_qualifier(working);
    if (qual_str.status == TRY_SUCCESS) {
        terminal.terminal.qualifier = qual_str.value.variant;
        working = strip_whitespace(strip(working, qual_str.value.str).value);
    }

    TryType_t type = parse_type(working);
    if (type.status == TRY_SUCCESS) {
        terminal.terminal.type = type.value;
        working = strip(working, type.value.str).value;
    }
    else if (type.status == TRY_ERROR) {
        GrammarPropagateError(type, output);
    }
    else {
        output.status = TRY_ERROR;
        output.error.location = working;
        output.error.desc = "No type found";
    }

    DerivedType_t *head_der = malloc(sizeof(DerivedType_t));
    memcpy(head_der, &terminal, sizeof(DerivedType_t));

    while (1) {
        working = strip_whitespace(working);

        TryConstString_t ast_str = find_string(working, const_string_from_cstr("*"));
        if (ast_str.status == TRY_SUCCESS) {
            working = strip(working, ast_str.value).value;
            DerivedType_t *next_der = malloc(sizeof(DerivedType_t));
            next_der->variant = DERIVED_TYPE_POINTER;
            next_der->pointer.inner_type = head_der;
            next_der->pointer.qualifier = QUALIFIER_NONE;
            qual_str = find_qualifier(working);
            if (qual_str.status == TRY_SUCCESS) {
                next_der->pointer.qualifier = qual_str.value.variant;
                working = strip(working, qual_str.value.str).value;
            }
            head_der = next_der;
            continue;
        }

        TryConstString_t brackets_str = find_last_closure_nesting_sensitive(working, '[', ']');
        if (brackets_str.status == TRY_ERROR) {
            GrammarPropagateError(brackets_str, output);
        }
        else if (brackets_str.status == TRY_SUCCESS) {
            working.end = brackets_str.value.begin;
            DerivedType_t *next_der = malloc(sizeof(DerivedType_t));
            next_der->variant = DERIVED_TYPE_ARRAY;
            next_der->array.inner_type = head_der;
            next_der->array.has_size = false;
            if (brackets_str.value.end - brackets_str.value.begin > 2) {
                next_der->array.has_size = true;
                ConstString_t size_str;
                size_str.begin = brackets_str.value.begin + 1;
                size_str.end = brackets_str.value.end - 1;
                next_der->array.size = new_alloc_const_string_from_const_str(size_str);
            }
            head_der = next_der;
            continue;
        }

        TryConstString_t parens_str = find_last_closure_nesting_sensitive(working, '(', ')');
        if (parens_str.status == TRY_ERROR) {
            GrammarPropagateError(parens_str, output);
        }
        else if (parens_str.status == TRY_SUCCESS) {
            if (parens_str.value.end != working.end) {
                ConstString_t leftover;
                leftover.begin = parens_str.value.end;
                leftover.end = working.end;
                TryConstString_t leftover_ws = find_whitespace(leftover);
                if (leftover_ws.status == TRY_NONE || leftover_ws.value.end != working.end) {
                    output.status = TRY_ERROR;
                    output.error.location = leftover;
                    output.error.desc = "Unexpected characters";
                    return output;
                }
            }
            ConstString_t contents;
            contents.begin = parens_str.value.begin + 1;
            contents.end = parens_str.value.end - 1;
            if (parens_str.value.begin == working.begin) {
                if (contents.begin == contents.end) {
                    output.status = TRY_ERROR;
                    output.error.location = parens_str.value;
                    output.error.desc = "Declaration cannot start with ()";
                    return output;
                }
                working = contents;
                continue;
            }
            working.end = parens_str.value.begin;
            DerivedType_t *next_der = malloc(sizeof(DerivedType_t));
            next_der->variant = DERIVED_TYPE_FUNCTION;
            next_der->function.params = NULL;
            next_der->function.return_type = head_der;
            VariableLinkedListNode_t **head_param = &next_der->function.params;
            while (contents.begin < contents.end) {
                ConstString_t param_str;
                param_str.begin = contents.begin;
                TryConstString_t comma = find_string_nesting_sensitive(contents, const_string_from_cstr(","));
                if (comma.status == TRY_ERROR) {
                    GrammarPropagateError(comma, output);
                }
                else if (comma.status == TRY_NONE) {
                    param_str.end = contents.end;
                    contents.begin = contents.end;
                }
                else {
                    param_str.end = comma.value.begin;
                    contents.begin = comma.value.end;
                }
                TryVariable_t param = parse_variable(param_str);
                if (param.status == TRY_ERROR) {
                    GrammarPropagateError(param, output);
                }
                else if (param.status == TRY_NONE) {
                    output.status = TRY_ERROR;
                    output.error.location = param_str;
                    output.error.desc = "Tried to parse a parameter, got NONE";
                    return output;
                }
                *head_param = malloc(sizeof(VariableLinkedListNode_t));
                (*head_param)->next = NULL;
                (*head_param)->value = param.value;
                head_param = &(*head_param)->next;
            }
            head_der = next_der;
            continue;
        }

        TryConstString_t id_str = find_identifier(working);
        GrammarPropagateError(id_str, output);
        if (id_str.status == TRY_SUCCESS) {
            working = strip_whitespace(strip(working, id_str.value).value);
            if (working.begin == working.end) {
                output.value.has_name = true;
                output.value.name = new_alloc_const_string_from_const_str(id_str.value);
                break;
            }
        }

        break;
    }

    working = strip_whitespace(working);
    if (working.begin != working.end) {
        output.status = TRY_ERROR;
        output.error.location = working;
        output.error.desc = "Unexpected characters";
        return output;
    }

    output.value.type = head_der;
    output.status = TRY_SUCCESS;

    return output;
}

size_t print_variable(char *buffer, const Variable_t* const var) {
    char swap1[0x10000];
    char swap2[0x10000];
    char swap3[0x10000];
    char *buffer_in = swap1;
    char *buffer_out = swap2;
    char *buffer_ex = swap3;

    buffer_in[0] = 0;
    if (var->has_name) {
        sprintf(buffer_out, "%.*s",
            (int)(var->name.end - var->name.begin), var->name.begin);
    }
    else {
        buffer_out[0] = 0;
    }

    DerivedType_t *der = var->type;
    while (der) {
        char *temp = buffer_in;
        buffer_in = buffer_out;
        buffer_out = temp;
        size_t num_chars = 0;
        switch (der->variant) {
            case DERIVED_TYPE_TERMINAL:
                print_type(buffer_ex, &der->terminal.type);
                if (buffer_in[0]) {
                    sprintf(buffer_out, "%s%s %s",
                        qualifier_strs[der->terminal.qualifier], buffer_ex, buffer_in);
                }
                else {
                    sprintf(buffer_out, "%s%s",
                        qualifier_strs[der->terminal.qualifier], buffer_ex);
                }
                der = NULL;
                break;
            case DERIVED_TYPE_POINTER:
                if (der->pointer.inner_type->variant == DERIVED_TYPE_ARRAY) {
                    sprintf(buffer_out, "(*%s%s)",
                        qualifier_strs[der->pointer.qualifier], buffer_in);
                }
                else {
                    sprintf(buffer_out, "*%s%s",
                        qualifier_strs[der->pointer.qualifier], buffer_in);
                }
                der = der->pointer.inner_type;
                break;
            case DERIVED_TYPE_ARRAY:
                if (der->array.has_size) {
                    sprintf(buffer_out, "%s[%.*s]",
                        buffer_in, (int)(der->array.size.end - der->array.size.begin), der->array.size.begin);
                }
                else {
                    sprintf(buffer_out, "%s[]", buffer_in);
                }
                der = der->array.inner_type;
                break;
            case DERIVED_TYPE_FUNCTION:
                if (find_identifier(const_string_from_cstr(buffer_in)).status == TRY_SUCCESS) {
                    num_chars += sprintf(buffer_out, "%s(", buffer_in);
                }
                else {
                    num_chars += sprintf(buffer_out, "(%s)(", buffer_in);
                }
                VariableLinkedListNode_t *param = der->function.params;
                while (param) {
                    num_chars += print_variable(buffer_out + num_chars, &param->value);
                    if (param->next) {
                        num_chars += sprintf(buffer_out + num_chars, ", ");
                    }
                    param = param->next;
                }
                sprintf(buffer_out + num_chars, ")");
                der = der->function.return_type;
                break;
        }
    }
    size_t output = strlen(buffer_out);
    memcpy(buffer, buffer_out, output + 1);
    return output;
}