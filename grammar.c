#include "grammar.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool try_consume_str(const char **const str, const char *const search) {
    const size_t search_len = strlen(search);
    if (!strncmp(*str, search, search_len)) {
        *str += search_len;
        return true;
    }
    else {
        return false;
    }
}

static bool try_rev_consume_str(const char **const end, const char *const search) {
    const size_t search_len = strlen(search);
    if (!strncmp(*end - search_len, search, search_len)) {
        *end -= search_len;
        return true;
    }
    else {
        return false;
    }
}

static const char *rev_find_char(const char c, const char *const str, const char *const end) {
    for (const char *it = end - 1; it >= str; --it) {
        if (*it == c) {
            return it;
        }
    }
    return NULL;
}

static const char *matching_paren(const char *str) {
    if (*str != '(') {
        return NULL;
    }
    const char *it = str + 1;
    int depth = 1;
    while (depth > 0) {
        if (*it == '(') {
            ++depth;
        }
        else if (*it == ')') {
            --depth;
        }
        else if (*it == 0) {
            return NULL;
        }
        ++it;
    }
    return it;
}

static const char *rev_matching_paren(const char *begin, const char *end) {
    if (*(end - 1) != ')') {
        return NULL;
    }
    const char *it = end - 1;
    int depth = 1;
    while (depth > 0 && it > begin) {
        --it;
        if (*it == ')') {
            ++depth;
        }
        else if (*it == '(') {
            --depth;
        }
    }
    return depth == 0 ? it : NULL;
}

static bool try_consume_parens(const char **begin, const char **end) {
    if (matching_paren(*begin) == *end) {
        ++(*begin);
        --(*end);
        return true;
    }
    else {
        return false;
    }
}

static uint64_t consume_whitespace(const char **const str) {
    const char *orig_str = *str;
    while (**str == ' ' || **str == '\t' || **str == '\n' || **str == '/') {
        if (**str == '/') {
            if (*(*str + 1) == '/') {
                (*str) += 2;
                while (**str != '\n' && **str != 0) {
                    ++(*str);
                }
                if (**str == 0) {
                    break;
                }
            }
            else if (*(*str + 1) == '*') {
                (*str) += 2;
                while ((**str != '*' || *(*str + 1) != '/') && *(*str + 1) != 0) {
                    ++(*str);
                }
                if (*(*str + 1) == 0) {
                    break;
                }
            }
        }
        ++(*str);
    }
    return *str - orig_str;
}

static uint64_t rev_consume_whitespace(const char **const end) {
    const char *orig_end = *end;
    while (*(*end - 1) == ' ' || *(*end - 1) == '\t' || *(*end - 1) == '\n') {
        --(*end);
    }
    return orig_end - *end;
} 

static char *try_consume_and_copy_identifier(const char **const str) {
    const char *orig_str = *str;
    if (('a' <= **str && **str <= 'z') || ('A' <= **str && **str <= 'Z') || **str == '_') {
        ++(*str);
    }
    else {
        return NULL;
    }
    while (('0' <= **str && **str <= '9') || ('a' <= **str && **str <= 'z') || ('A' <= **str && **str <= 'Z') || **str == '_') {
        ++(*str);
    }
    const size_t len = *str - orig_str;
    char *output = (char *)malloc(len + 1);
    memcpy(output, orig_str, len);
    output[len] = 0;
    return output;
}

typedef TryGrammar(DerivedType_t*) TryDerivedType_t;

TryDerivedType_t try_consume_derived_type(const char **const begin, const char **const end) {
    TryDerivedType_t output;
    output.success = true;
    output.value = NULL;

    if (*begin == *end) {
        return output;
    }

    while (consume_whitespace(begin) || rev_consume_whitespace(end) || try_consume_parens(begin, end));
    const char *working_begin = *begin;
    const char *working_end = *end;

    // Try parsing a pointer
    if (*working_begin == '*') {
        working_begin++;
        consume_whitespace(&working_begin);
        const char *restore_begin = working_begin;
        ConstVolatileQualification_t qualification = NONE;
        if (try_consume_str(&working_begin, "const")) {
            if (consume_whitespace(&working_begin) || *working_begin == '*' || *working_begin == '(') {
                qualification = CONST;
            }
            else {
                working_begin = restore_begin;
            }
        }
        else if (try_consume_str(&working_begin, "volatile")) {
            if (consume_whitespace(&working_begin) || *working_begin == '*' || *working_begin == '(') {
                qualification = VOLATILE;
            }
            else {
                working_begin = restore_begin;
            }
        }
        *begin = working_begin;
        output.value = malloc(sizeof(DerivedType_t));
        output.value->variant = POINTER_TO_TYPE;
        output.value->pointer.child_type = NULL;
        output.value->pointer.qualification = qualification;
        return output;
    }

    // Try parsing an array
    if (*(working_end - 1) == ']') {
        const char *closed_bracket_it = working_end - 1;
        const char *open_bracket_it = rev_find_char('[', working_begin, closed_bracket_it);
        if (!open_bracket_it) {
            output.success = false;
            output.error.it = closed_bracket_it;
            output.error.desc = "No '[' to match ']'";
            return output;
        }
        *end = open_bracket_it;
        ++open_bracket_it;
        consume_whitespace(&open_bracket_it);

        bool has_value = false;
        uint64_t size = 0;
        assert(open_bracket_it <= closed_bracket_it);
        if (open_bracket_it == closed_bracket_it) {
            has_value = false;
        }
        else {
            has_value = true;
            const char *size_end_it = open_bracket_it;
            size = consume_uint64(&size_end_it);
            consume_whitespace(&size_end_it);
            if (size_end_it != closed_bracket_it) {
                output.success = false;
                output.error.it = size_end_it;
                output.error.desc = "Brackets must contain either nothing or a positive integer";
                return output;
            }
        }

        output.value = malloc(sizeof(DerivedType_t));
        output.value->variant = ARRAY_OF_TYPE;
        output.value->array.size.has_value = has_value;
        output.value->array.size.value = size;
        output.value->array.child_type = NULL;

        return output;
    }

    // Try parsing a function
    if (*(working_end - 1) == ')') {
        const char *params_begin = rev_matching_paren(working_begin, working_end);
        const char *cutoff = params_begin;
        if (!params_begin) {
            output.success = false;
            output.error.it = working_end - 1;
            output.error.desc = "No '(' to match ')'";
            return output;
        }

        const char *params_end = working_end;
        assert(params_begin != working_begin);
        try_consume_parens(&params_begin, &params_end);

        output.value = malloc(sizeof(DerivedType_t));
        output.value->variant = FUNCTION_TYPE;
        output.value->function.params = NULL;
        output.value->function.return_type = NULL;

        VariableLinkedList_t *params_head = output.value->function.params;

        const char *param_begin = params_begin;
        const char *param_end = param_begin;
        while (param_end < params_end) {
            // Find the next comma
            while (*param_end != ',' && param_end < params_end) {
                if (*param_end == '(') {
                    const char *closing_paren = matching_paren(param_end);
                    if (!closing_paren) {
                        free(output.value);
                        output.value = NULL;
                        output.success = false;
                        output.error.it = param_end;
                        output.error.desc = "No ')' to match '('";
                        return output;
                    }
                    param_end = closing_paren;
                }
                else {
                    ++param_end;
                }
            }
            TryVariable_t var = parse_variable(param_begin, param_end);
            if (!var.success) {
                free(output.value);
                output.value = NULL;
                output.success = false;
                output.error.it = var.error.it;
                output.error.desc = var.error.desc;
                return output;
            }
            VariableLinkedList_t *next_param = malloc(sizeof(VariableLinkedList_t));
            next_param->value = malloc(sizeof(Variable_t));
            next_param->next = NULL;
            memcpy(next_param->value, &var.value, sizeof(Variable_t));
            if (!output.value->function.params) {
                output.value->function.params = next_param;
                params_head = next_param;
            }
            else {
                params_head->next = next_param;
                params_head = next_param;
            }

            param_begin = param_end + 1;
            param_end = param_begin;
        }

        *end = cutoff;
        
        return output;
    }

    return output;
}

TryVariable_t parse_variable(const char *const begin, const char *const end) {
    TryVariable_t output;
    output.success = true;
    output.value.name = NULL;

    // Get rid of all beginning and ending whitespace
    const char *working_begin = begin;
    consume_whitespace(&working_begin);
    const char *restore_begin = working_begin;
    const char *working_end = end;
    rev_consume_whitespace(&working_end);

    // The root must have a terminal type
    DerivedType_t *terminal = malloc(sizeof(DerivedType_t));
    terminal->variant = TERMINAL_TYPE;

    // Check for const or volatile
    if (try_consume_str(&working_begin, "const")) {
        if (consume_whitespace(&working_begin)) {
            terminal->terminal.qualification = CONST;
        }
        else {
            working_begin = restore_begin;
        }
    }
    else if (try_consume_str(&working_begin, "volatile")) {
        if (consume_whitespace(&working_begin)) {
            terminal->terminal.qualification = VOLATILE;
        }
        else {
            working_begin = restore_begin;
        }
    }
    terminal->terminal.name = try_consume_and_copy_identifier(&working_begin);
    output.value.type = terminal;

    DerivedType_t *current_derived = terminal;
    while (current_derived) {
        TryDerivedType_t next_derived = try_consume_derived_type(&working_begin, &working_end);
        if (next_derived.success) {
            if (next_derived.value) {
                switch (next_derived.value->variant) {
                    case TERMINAL_TYPE: break;
                    case POINTER_TO_TYPE:
                        next_derived.value->pointer.child_type = current_derived;
                        output.value.type = next_derived.value;
                        break;
                    case ARRAY_OF_TYPE:
                        next_derived.value->array.child_type = current_derived;
                        output.value.type = next_derived.value;
                        break;
                    case FUNCTION_TYPE:
                        next_derived.value->function.return_type = current_derived;
                        output.value.type = next_derived.value;
                        break;
                }
            }
            else {
                output.value.name = try_consume_and_copy_identifier(&working_begin);
                if (working_begin != working_end) {
                    output.success = false;
                    output.error.it = working_begin;
                    output.error.desc = "Too many characters";
                    return output;
                }
            }
            current_derived = next_derived.value;
        }
        else {
            output.success = false;
            output.error.it = next_derived.error.it;
            output.error.desc = next_derived.error.desc;
            return output;
        }
    }

    return output;
}

void print_variable(const Variable_t *const var, char *buffer) {
    char swap1[0x1000];
    char swap2[0x1000];
    char swap3[0x1000];
    char *print_out = swap1;
    char *print_in = swap2;
    if (var->name) {
        sprintf(print_out, "%s", var->name);
    }
    else {
        sprintf(print_out, "");
    }
    DerivedType_t *der = var->type;
    DerivedType_t *prev_der = NULL;
    while (der) {
        print_in = (print_in == swap1) ? swap2 : swap1;
        print_out = (print_out == swap1) ? swap2 : swap1;
        DerivedType_t *current_der = der;
        if (der->variant == TERMINAL_TYPE) {
            if (strlen(print_in)) {
                switch (der->terminal.qualification) {
                    case NONE:
                        sprintf(print_out, "%s %s", der->terminal.name, print_in);
                        break;
                    case CONST:
                        sprintf(print_out, "const %s %s", der->terminal.name, print_in);
                        break;
                    case VOLATILE:
                        sprintf(print_out, "volatile %s %s", der->terminal.name, print_in);
                        break;
                }
            }
            else {
                switch (der->terminal.qualification) {
                    case NONE:
                        sprintf(print_out, "%s", der->terminal.name);
                        break;
                    case CONST:
                        sprintf(print_out, "const %s", der->terminal.name);
                        break;
                    case VOLATILE:
                        sprintf(print_out, "volatile %s", der->terminal.name);
                        break;
                }
            }
            break;
        }
        else if (der->variant == POINTER_TO_TYPE) {
            switch (der->pointer.qualification) {
                case NONE:
                    sprintf(print_out, "*%s", print_in);
                    break;
                case CONST:
                    sprintf(print_out, "*const %s", print_in);
                    break;
                case VOLATILE:
                    sprintf(print_out, "*volatile %s", print_in);
                    break;
            }
            der = der->pointer.child_type;
        }
        else if (der->variant == ARRAY_OF_TYPE) {
            if (prev_der && prev_der->variant == POINTER_TO_TYPE) {
                sprintf(swap3, "(%s)", print_in);
            }
            else {
                sprintf(swap3, "%s", print_in);
            }
            if (der->array.size.has_value) {
                sprintf(print_out, "%s[%lu]", swap3, der->array.size.value);
            }
            else {
                sprintf(print_out, "%s[]", swap3);
            }
            der = der->array.child_type;
        }
        else if (der->variant == FUNCTION_TYPE) {
            size_t n_written = 0;
            if (prev_der) {
                n_written = sprintf(print_out, "(%s)(", print_in);
            }
            else {
                n_written = sprintf(print_out, "%s(", print_in);
            }
            char *it = print_out + n_written;
            VariableLinkedList_t *param = der->function.params;
            while (param) {
                print_variable(param->value, swap3);
                it += sprintf(it, param->next ? "%s, " : "%s", swap3);
                param = param->next;
            }
            sprintf(it, ")");
            der = der->function.return_type;
        }
        prev_der = current_der;
    }
    memcpy(buffer, print_out, strlen(print_out));
}