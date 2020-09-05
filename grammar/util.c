#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ConstString_t const_string_from_cstr(const char *const str) {
    size_t len = strlen(str);

    ConstString_t output;
    output.begin = str;
    output.end = str + len;
    return output;
}

void print_const_string(const ConstString_t str, const char *message) {
    if (message) {
        printf("%s: \"%.*s\"\n", message, (int)(str.end - str.begin), str.begin);
    }
    else {
        printf("\"%.*s\"\n", (int)(str.end - str.begin), str.begin);
    }
}

AConstString_t new_alloc_const_string_from_cstr(const char *const str) {
    size_t len = strlen(str);
    char *buffer = malloc(len + 1);
    memcpy(buffer, str, len);
    buffer[len] = 0;
    
    AConstString_t output;
    output.begin = buffer;
    output.end = buffer + len;
    return output;
}

AConstString_t new_alloc_const_string_from_const_str(const ConstString_t str) {
    size_t len = str.end - str.begin;
    char *buffer = malloc(len + 1);
    memcpy(buffer, str.begin, len);
    buffer[len] = 0;

    AConstString_t output;
    output.begin = buffer;
    output.end = buffer + len;
    return output;
}

void free_alloc_const_string(AConstString_t *const str) {
    free(str->begin);
    str->begin = NULL;
    str->end = NULL;
}

TryConstString_t strip(const ConstString_t parent, const ConstString_t child) {
    TryConstString_t output;
    output.status = TRY_SUCCESS;
    output.value = parent;
    if (parent.begin == child.begin) {
        if (child.end > parent.end) {
            output.status = TRY_ERROR;
            output.error.location = child;
            output.error.desc = "strip(): child is larger than parent";
            return output;
        }
        output.value.begin = child.end;
    }
    else if (parent.end == child.end) {
        if (child.begin < parent.begin) {
            output.status = TRY_ERROR;
            output.error.location = child;
            output.error.desc = "strip(): child is larger than parent";
            return output;
        }
        output.value.end = child.begin;
    }
    else {
        output.status = TRY_ERROR;
        output.error.location = parent;
        output.error.desc = "strip(): parent and child do not share a boundary";
    }
    return output;
}

TryConstString_t find_whitespace(const ConstString_t str) {
    assert(str.begin <= str.end);
    TryConstString_t output;
    const char *it = str.begin;
    while (it < str.end && (*it == ' ' || *it == '\t' || *it == '\n')) {
        ++it;
    }
    if (it == str.begin) {
        output.status = TRY_NONE;
    }
    else {
        output.status = TRY_SUCCESS;
        output.value.begin = str.begin;
        output.value.end = it;
    }
    return output;
}

ConstString_t strip_whitespace(const ConstString_t str) {
    TryConstString_t ws = find_whitespace(str);
    if (ws.status == TRY_NONE) {
        return str;
    }
    else {
        TryConstString_t res = strip(str, ws.value);
        assert(res.status == TRY_SUCCESS);
        return res.value;
    }
}

TryConstString_t find_string(const ConstString_t str, const ConstString_t pattern) {
    TryConstString_t output;
    size_t len = pattern.end - pattern.begin;
    if (len > (str.end - str.begin) || strncmp(str.begin, pattern.begin, len)) {
        output.status = TRY_NONE;
        return output;
    }
    output.status = TRY_SUCCESS;
    output.value.begin = str.begin;
    output.value.end = str.begin + len;
    return output;
}

TryConstString_t find_first_string(const ConstString_t str, const ConstString_t pattern) {
    TryConstString_t output;
    ConstString_t working = str;
    size_t len = pattern.end - pattern.begin;
    while (working.begin + len < working.end) {
        TryConstString_t find = find_string(working, pattern);
        if (find.status == TRY_SUCCESS) {
            return find;
        }
        else {
            ++working.begin;
        }
    }
    output.status = TRY_NONE;
    return output;
}

TryConstString_t find_identifier(const ConstString_t str) {
    TryConstString_t output;
    const char *it = str.begin;
    if (str.begin == str.end || (*it != '_' && !('a' <= *it && *it <= 'z') && !('A' <= *it && *it <= 'Z'))) {
        output.status = TRY_NONE;
        return output;
    }
    ++it;
    while (it < str.end && (*it == '_' || ('a' <= *it && *it <= 'z') || ('A' <= *it && *it <= 'Z') || ('0' <= *it && *it <= '9'))) {
        ++it;
    }
    output.status = TRY_SUCCESS;
    output.value.begin = str.begin;
    output.value.end = it;
    return output;
}

TryIntegerLiteral_t find_integer(const ConstString_t str) {
    TryIntegerLiteral_t output;
    output.value.integer = 0;
    const char *it = str.begin;
    while (it < str.end && '0' <= *it && *it <= '9') {
        output.value.integer *= 10;
        output.value.integer += *it - '0';
        ++it;
    }
    if (it == str.begin) {
        output.status = TRY_NONE;
        return output;
    }
    output.status = TRY_SUCCESS;
    output.value.str.begin = str.begin;
    output.value.str.end = it;
    return output;
}

TryConstString_t find_closing(const ConstString_t str, const char opening, const char closing) {
    TryConstString_t output;
    if (str.begin == str.end || *str.begin != opening) {
        output.status = TRY_NONE;
        return output;
    }
    size_t depth = 1;
    const char *it = str.begin + 1;
    while (depth > 0 && it < str.end) {
        if (*it == opening) {
            ++depth;
        }
        else if (*it == closing) {
            --depth;
        }
        ++it;
    }
    if (depth > 0) {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "No closing character";
    }
    else {
        output.status = TRY_SUCCESS;
        output.value.begin = str.begin;
        output.value.end = it;
    }
    return output;
}

TryConstString_t find_string_lit(const ConstString_t str, const char quote, const char escape) {
    TryConstString_t output;
    if (str.begin == str.end || *str.begin != quote) {
        output.status = TRY_NONE;
        return output;
    }
    const char *it = str.begin + 1;
    while (*it != quote && it < str.end) {
        if (*it == escape) {
            ++it;
        }
        ++it;
    }
    if (it == str.end) {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "No closing quote";
    }
    else {
        output.status = TRY_SUCCESS;
        output.value.begin = str.begin;
        output.value.end = it + 1;
    }
    return output;
}

TryConstString_t find_string_nesting_sensitive(const ConstString_t str, const ConstString_t pattern) {
    TryConstString_t output;
    ConstString_t working = str;
    while (working.begin < working.end) {
        TryConstString_t match = find_string(working, pattern);
        if (match.status == TRY_SUCCESS) {
            output.status = TRY_SUCCESS;
            output.value = match.value;
            return output;
        }

        typedef struct {
            const char *const pair;
            TryConstString_t (*const func)(const ConstString_t, const char, const char);
        } Predicate_t;
        static const Predicate_t preds[] = {
            {"()", find_closing},
            {"[]", find_closing},
            {"{}", find_closing},
            {"\"\\", find_string_lit},
            {"'\\", find_string_lit},
            {NULL, NULL}
        };
        const Predicate_t *pred = preds;
        while (pred->pair) {
            TryConstString_t closure = pred->func(working, pred->pair[0], pred->pair[1]);
            if (closure.status == TRY_SUCCESS) {
                working = strip(working, closure.value).value;
                break;
            }
            else if (closure.status == TRY_ERROR) {
                GrammarPropagateError(closure, output);
            }
            ++pred;
        }
        if (!pred->pair) {
            ++working.begin;
        }
    }
    output.status = TRY_NONE;
    return output;
}

TryConstString_t find_last_closure_nesting_sensitive(const ConstString_t str, const char opening, const char closing) {
    TryConstString_t output;
    output.status = TRY_NONE;
    ConstString_t working = str;
    while (working.begin < working.end) {
        // Try to another opening
        char pattern[2] = { opening, 0 };
        TryConstString_t opening_str = find_string_nesting_sensitive(working, const_string_from_cstr(pattern));
        if (opening_str.status == TRY_NONE) {
            return output;
        }
        working.begin = opening_str.value.begin;
        TryConstString_t closure_str = find_closing(working, opening, closing);
        if (closure_str.status == TRY_ERROR) {
            GrammarPropagateError(closure_str, output);
        }
        else if (closure_str.status == TRY_NONE) {
            output.status = TRY_ERROR;
            output.error.location = working;
            output.error.desc = "Could not find a parenthesis we thought we already found";
            return output;
        }
        output.status = TRY_SUCCESS;
        output.value = closure_str.value;
        working.begin = closure_str.value.end;
    }
    return output;
}