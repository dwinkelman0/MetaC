#include "grammar.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool try_consume_str(ConstString *const str, const char *const search) {
    const size_t str_len = str->end - str->begin;
    const size_t search_len = strlen(search);
    if (str_len >= search_len && !strncmp(str->begin, search, search_len)) {
        str->begin += search_len;
        return true;
    }
    else {
        return false;
    }
}

static bool try_rev_consume_str(ConstString *const str, const char *const search) {
    const size_t str_len = str->end - str->begin;
    const size_t search_len = strlen(search);
    if (str_len >= search_len && !strncmp(str->end - search_len, search, search_len)) {
        str->end -= search_len;
        return true;
    }
    else {
        return false;
    }
}

static const char *rev_find_char(const char c, const ConstString *const str) {
    for (const char *it = str->end - 1; it >= str->begin; --it) {
        if (*it == c) {
            return it;
        }
    }
    return NULL;
}

static const char *matching_paren(const ConstString *const str) {
    if (*str->begin != '(') {
        return NULL;
    }
    const char *it = str->begin + 1;
    int depth = 1;
    while (depth > 0) {
        if (it == str->end) {
            return NULL;
        }
        if (*it == '(') {
            ++depth;
        }
        else if (*it == ')') {
            --depth;
        }
        ++it;
    }
    return it;
}

// !!! Code Smell
static const char *rev_matching_paren(const ConstString *const str) {
    if (*(str->end - 1) != ')') {
        return NULL;
    }
    const char *it = str->end - 1;
    int depth = 1;
    while (depth > 0) {
        --it;
        if (it < str->begin) {
            return NULL;
        }
        if (*it == ')') {
            ++depth;
        }
        else if (*it == '(') {
            --depth;
        }
    }
    return depth == 0 ? it : NULL;
}

static bool try_consume_parens(ConstString *const str) {
    if (matching_paren(str) == str->end) {
        ++str->begin;
        ++str->end;
        return true;
    }
    else {
        return false;
    }
}

static uint64_t consume_whitespace(ConstString *const str) {
    const char *orig_str = str->begin;
    while (*str->begin == ' ' || *str->begin == '\t' || *str->begin == '\n' || *str->begin == '/') {
        if (*str->begin == '/') {
            if (*(str->begin + 1) == '/') {
                str->begin += 2;
                while (*str->begin != '\n' && *str->begin != 0) {
                    ++str->begin;
                }
                if (*str->begin == 0) {
                    break;
                }
            }
            else if (*(str->begin + 1) == '*') {
                str->begin += 2;
                while ((*str->begin != '*' || *(str->begin + 1) != '/') && *(str->begin + 1) != 0) {
                    ++str->begin;
                }
                if (*(str->begin + 1) == 0) {
                    break;
                }
            }
        }
        ++str->begin;
    }
    return str->begin - orig_str;
}

static uint64_t rev_consume_whitespace(ConstString *const str) {
    const char *orig_end = str->end;
    while (*(str->end - 1) == ' ' || *(str->end - 1) == '\t' || *(str->end - 1) == '\n') {
        --str->end;
    }
    return orig_end - str->end;
} 

static char *try_consume_and_copy_identifier(ConstString *const str) {
    const char *orig_str = str->begin;
    if (('a' <= *str->begin && *str->begin <= 'z') || ('A' <= *str->begin && *str->begin <= 'Z') || *str->begin == '_') {
        ++str->begin;
    }
    else {
        return NULL;
    }
    while (('0' <= *str->begin && *str->begin <= '9') || ('a' <= *str->begin && *str->begin <= 'z') || ('A' <= *str->begin && *str->begin <= 'Z') || *str->begin == '_') {
        ++str->begin;
    }
    const size_t len = str->begin - orig_str;
    char *output = (char *)malloc(len + 1);
    memcpy(output, orig_str, len);
    output[len] = 0;
    return output;
}

typedef TryGrammar(DerivedType_t*) TryDerivedType_t;

TryDerivedType_t try_consume_derived_type(ConstString *const str) {
    TryDerivedType_t output;
    output.success = true;
    output.value = NULL;

    if (str->begin == str->end) {
        return output;
    }

    while (consume_whitespace(str) || rev_consume_whitespace(str) || try_consume_parens(str));
    ConstString working = *str;

    // Try parsing a pointer
    if (*working.begin == '*') {
        working.begin++;
        consume_whitespace(&working);
        ConstString restore = working;
        ConstVolatileQualification_t qualification = NONE;
        if (try_consume_str(&working, "const")) {
            if (consume_whitespace(&working) || *working.begin == '*' || *working.begin == '(') {
                qualification = CONST;
            }
            else {
                working.begin = restore.begin;
            }
        }
        else if (try_consume_str(&working, "volatile")) {
            if (consume_whitespace(&working) || *working.begin == '*' || *working.begin == '(') {
                qualification = VOLATILE;
            }
            else {
                working.begin = restore.begin;
            }
        }
        str->begin = working.begin;
        output.value = malloc(sizeof(DerivedType_t));
        output.value->variant = POINTER_TO_TYPE;
        output.value->pointer.child_type = NULL;
        output.value->pointer.qualification = qualification;
        return output;
    }

    // Try parsing an array
    if (*(working.end - 1) == ']') {
        working.end -= 1;
        working.begin = rev_find_char('[', &working);
        if (!working.begin) {
            output.success = false;
            output.error.it = working.end;
            output.error.desc = "No '[' to match ']'";
            return output;
        }
        str->end = working.begin;
        working.begin++;
        consume_whitespace(&working);

        bool has_value = false;
        uint64_t size = 0;
        if (working.begin == working.end) {
            has_value = false;
        }
        else {
            has_value = true;
            size = consume_uint64(&working.begin);
            consume_whitespace(&working);
            if (working.begin != working.end) {
                output.success = false;
                output.error.it = working.begin;
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
    if (*(working.end - 1) == ')') {
        working.end = rev_matching_paren(&working);
        const char *cutoff = working.begin;
        if (!working.begin) {
            output.success = false;
            output.error.it = working.end - 1;
            output.error.desc = "No '(' to match ')'";
            return output;
        }
        try_consume_parens(&working);

        output.value = malloc(sizeof(DerivedType_t));
        output.value->variant = FUNCTION_TYPE;
        output.value->function.params = NULL;
        output.value->function.return_type = NULL;

        VariableLinkedList_t *params_head = output.value->function.params;

        ConstString param = working;
        param.end = working.end;
        while (param.end < working.end) {
            // Find the next comma
            while (*param.end != ',' && param.end < working.end) {
                if (*param.end == '(') {
                    const char *closing_paren = matching_paren(&param);
                    if (!closing_paren) {
                        free(output.value);
                        output.value = NULL;
                        output.success = false;
                        output.error.it = param.end;
                        output.error.desc = "No ')' to match '('";
                        return output;
                    }
                    param.end = closing_paren;
                }
                else {
                    ++param.end;
                }
            }
            TryVariable_t var = parse_variable(&param);
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

            param.begin = param.end + 1;
            param.end = param.begin;
        }

        str->end = cutoff;
        
        return output;
    }

    return output;
}

TryVariable_t parse_variable(const ConstString *const input) {
    TryVariable_t output;
    output.success = true;
    output.value.name = NULL;

    // Get rid of all beginning and ending whitespace
    ConstString working = *input;
    consume_whitespace(&working);
    rev_consume_whitespace(&working);
    ConstString restore = working;

    // The root must have a terminal type
    DerivedType_t *terminal = malloc(sizeof(DerivedType_t));
    terminal->variant = TERMINAL_TYPE;

    // Check for const or volatile
    if (try_consume_str(&working, "const")) {
        if (consume_whitespace(&working)) {
            terminal->terminal.qualification = CONST;
        }
        else {
            working = restore;
        }
    }
    else if (try_consume_str(&working, "volatile")) {
        if (consume_whitespace(&working)) {
            terminal->terminal.qualification = VOLATILE;
        }
        else {
            working = restore;
        }
    }
    terminal->terminal.name = try_consume_and_copy_identifier(&working);
    output.value.type = terminal;

    DerivedType_t *current_derived = terminal;
    while (current_derived) {
        TryDerivedType_t next_derived = try_consume_derived_type(&working);
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
                output.value.name = try_consume_and_copy_identifier(&working);
                if (working.begin != working.end) {
                    output.success = false;
                    output.error.it = working.begin;
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