#include "grammar.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    PrimitiveVariant_t variant;
    const char *words[4];
} PrimitiveMapping_t;
typedef struct {
    PrimitiveVariant_t variant;
    ConstString_t str;
} PrimitiveLocation_t;
typedef GrammarTryType(PrimitiveLocation_t) TryPrimitiveLocation_t;

const static size_t n_mappings = 18;
const static PrimitiveMapping_t primitive_map[] = {
    {PRIMITIVE_VOID, {"void", NULL}},
    {PRIMITIVE_CHAR, {"char", NULL}},
    {PRIMITIVE_CHAR, {"signed", "char", NULL}},
    {PRIMITIVE_UNSIGNED_CHAR, {"unsigned", "char", NULL}},
    {PRIMITIVE_SHORT, {"short", NULL}},
    {PRIMITIVE_SHORT, {"signed", "short", NULL}},
    {PRIMITIVE_UNSIGNED_SHORT, {"unsigned", "short", NULL}},
    {PRIMITIVE_INT, {"short", NULL}},
    {PRIMITIVE_INT, {"signed", "int", NULL}},
    {PRIMITIVE_UNSIGNED_INT, {"unsigned", "int", NULL}},
    {PRIMITIVE_LONG_LONG, {"long", "long", NULL}},
    {PRIMITIVE_LONG_LONG, {"signed", "long", "long", NULL}},
    {PRIMITIVE_UNSIGNED_LONG_LONG, {"unsigned", "long", "long", NULL}},
    {PRIMITIVE_LONG, {"long", NULL}},
    {PRIMITIVE_LONG, {"signed", "long", NULL}},
    {PRIMITIVE_UNSIGNED_LONG, {"unsigned", "long", NULL}},
    {PRIMITIVE_FLOAT, {"float", NULL}},
    {PRIMITIVE_DOUBLE, {"double", NULL}},
};

const static char *type_strs[] = {
    "struct",
    "union",
    "enum"
};

const static char *primitive_strs[] = {
    "void",
    "char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
    "float",
    "double"
};

/**
 * Check whether a primitive exists at the beginning of the string; if so,
 * return the string and variant of the primitive
 *  SUCCESS: a primitive was found
 *  NONE: a primitive was not found
 */
static TryPrimitiveLocation_t find_primitive(const ConstString_t str) {
    TryPrimitiveLocation_t output;
    TryConstString_t word;
    for (int i = 0; i < n_mappings; ++i) {
        ConstString_t working = str;
        const char *const *word = primitive_map[i].words;
        while (*word) {
            TryConstString_t try_word = find_string(working, const_string_from_cstr(*word));
            if (try_word.status != TRY_SUCCESS) {
                break;
            }
            else {
                working = strip_whitespace(strip(working, try_word.value).value);
            }
            ++word;
        }
        if (!*word) {
            output.status = TRY_SUCCESS;
            output.value.variant = primitive_map[i].variant;
            output.value.str.begin = str.begin;
            output.value.str.end = working.begin;
            return output;
        }
    }
    output.status = TRY_NONE;
    return output;
}

TryEnumField_t parse_enum(const ConstString_t str, const int expected_value) {
    TryEnumField_t output;
    ConstString_t working = strip_whitespace(str);
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }
    TryConstString_t name = find_identifier(working);
    if (name.status == TRY_NONE) {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "Enum field needs a name";
        return output;
    }
    output.value.name = new_alloc_const_string_from_const_str(name.value);
    working = strip_whitespace(strip(working, name.value).value);
    TryConstString_t equals = find_string(working, const_string_from_cstr("="));
    if (equals.status == TRY_SUCCESS) {
        working = strip_whitespace(strip(working, equals.value).value);
        TryIntegerLiteral_t integer = find_integer(working);
        if (integer.status == TRY_NONE) {
            output.status = TRY_ERROR;
            output.error.location = working;
            output.error.desc = "Expected a positive, base-10 integer";
            return output;
        }
        working = strip(working, integer.value.str).value;
        output.value.value = integer.value.integer;
    }
    else {
        output.value.value = expected_value;
    }
    working = strip_whitespace(working);
    if (working.begin != working.end) {
        output.status = TRY_ERROR;
        output.error.location = working;
        output.error.desc = "Unexpected characters in enum field";
    }
    else {
        output.status = TRY_SUCCESS;
    }
    return output;
}

TryType_t parse_type(const ConstString_t str) {
    TryType_t output;

    ConstString_t working = strip_whitespace(str);
    if (working.begin == working.end) {
        output.status = TRY_NONE;
        return output;
    }

    output.value.variant = TYPE_NAMED;
    TryConstString_t type_spec;
    TryPrimitiveLocation_t primitive_spec;
    TryConstString_t id_spec;
    if ((type_spec = find_string(working, const_string_from_cstr("struct"))).status == TRY_SUCCESS) {
        output.value.variant = TYPE_STRUCT;
        working = strip(working, type_spec.value).value;
    }
    else if ((type_spec = find_string(working, const_string_from_cstr("union"))).status == TRY_SUCCESS) {
        output.value.variant = TYPE_UNION;
        working = strip(working, type_spec.value).value;
    }
    else if ((type_spec = find_string(working, const_string_from_cstr("enum"))).status == TRY_SUCCESS) {
        output.value.variant = TYPE_ENUM;
        working = strip(working, type_spec.value).value;
    }
    else if ((primitive_spec = find_primitive(working)).status == TRY_SUCCESS) {
        output.value.variant = TYPE_PRIMITIVE;
        output.value.primitive = primitive_spec.value.variant;
        output.value.str.begin = str.begin;
        output.value.str.end = primitive_spec.value.str.end;
        output.status = TRY_SUCCESS;
        return output;
    }
    else if ((id_spec = find_identifier(working)).status == TRY_SUCCESS) {
        output.value.variant = TYPE_NAMED;
        output.value.named = new_alloc_const_string_from_const_str(id_spec.value);
        output.value.str.begin = str.begin;
        output.value.str.end = id_spec.value.end;
        output.status = TRY_SUCCESS;
        return output;
    }
    else {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "Expected an identifier";
        return output;
    }

    TryConstString_t ws = find_whitespace(working);
    output.value.compound.has_name = false;
    if (ws.status == TRY_SUCCESS) {
        working = strip(working, ws.value).value;
        TryConstString_t name = find_identifier(working);
        if (name.status == TRY_SUCCESS) {
            output.value.compound.name = new_alloc_const_string_from_const_str(name.value);
            output.value.compound.has_name = true;
            output.value.str.end = name.value.end;
            working = strip(working, name.value).value;
            working = strip_whitespace(working);
        }
    }

    TryConstString_t contents = find_closing(working, '{', '}');
    GrammarPropagateError(contents, output);
    if (contents.status == TRY_SUCCESS) {
        ConstString_t braces;
        braces.begin = contents.value.begin + 1;
        braces.end = contents.value.end - 1;
        working = strip(working, contents.value).value;
        output.value.str.end = contents.value.end;
        output.value.compound.is_definition = true;
        output.value.compound.su_fields = NULL;
        output.value.compound.e_fields = NULL;

        if (output.value.variant == TYPE_STRUCT || output.value.variant == TYPE_UNION) {
            VariableLinkedListNode_t **head_field = &output.value.compound.su_fields;
            while (braces.begin < braces.end) {
                TryConstString_t semicolon = find_string_nesting_sensitive(braces, const_string_from_cstr(";"));
                if (semicolon.status == TRY_SUCCESS) {
                    ConstString_t field;
                    field.begin = braces.begin;
                    field.end = semicolon.value.begin;
                    braces.begin = semicolon.value.end;
                    TryVariable_t var = parse_variable(field);
                    if (var.status == TRY_ERROR) {
                        GrammarPropagateError(var, output);
                    }
                    else if (var.status == TRY_NONE) {
                        output.status = TRY_ERROR;
                        output.error.location = field;
                        output.error.desc = "Expected a field declaration";
                        return output;
                    }
                    *head_field = malloc(sizeof(VariableLinkedListNode_t));
                    (*head_field)->next = NULL;
                    (*head_field)->value = var.value;
                    head_field = &(*head_field)->next;
                }
                else {
                    braces = strip_whitespace(braces);
                    if (braces.begin != braces.end) {
                        output.status = TRY_ERROR;
                        output.error.location = braces;
                        output.error.desc = "Unexpected characters in struct/union definition";
                        return output;
                    }
                }
            }
        }
        else {
            EnumFieldLinkedListNode_t **head_field = &output.value.compound.e_fields;
            int64_t expected_value = 0;
            while (braces.begin < braces.end) {
                TryConstString_t comma = find_string_nesting_sensitive(braces, const_string_from_cstr(","));
                ConstString_t field;
                if (comma.status == TRY_SUCCESS) {
                    field.begin = braces.begin;
                    field.end = comma.value.begin;
                    braces.begin = comma.value.end;
                }
                else {
                    field = braces;
                    braces.begin = braces.end;
                }
                TryEnumField_t pair = parse_enum(field, expected_value);
                if (pair.status == TRY_ERROR) {
                    GrammarPropagateError(pair, output);
                }
                else if (pair.status == TRY_NONE) {
                    output.status = TRY_ERROR;
                    output.error.location = field;
                    output.error.desc = "Expected a field declaration";
                    return output;
                }
                *head_field = malloc(sizeof(EnumFieldLinkedListNode_t));
                (*head_field)->next = NULL;
                (*head_field)->value = pair.value;
                head_field = &(*head_field)->next;
                expected_value = pair.value.value + 1;
            }
        }
    }
    else {
        output.value.compound.is_definition = false;
    }

    if (!output.value.compound.is_definition && !output.value.compound.has_name) {
        output.status = TRY_ERROR;
        output.error.location = str;
        output.error.desc = "struct/union/enum needs a name or a definition";
        return output;
    }

    output.value.str.begin = str.begin;
    output.status = TRY_SUCCESS;
    return output;
}

size_t print_type(char *buffer, const Type_t *const type) {
    size_t num_chars = 0;
    if (type->variant == TYPE_PRIMITIVE) {
        num_chars += sprintf(buffer, "%s", primitive_strs[type->primitive]);
    }
    else if (type->variant == TYPE_NAMED) {
        num_chars += sprintf(buffer, "%s", type->named.begin);
    }
    else {
        if (type->compound.is_definition) {
            char field_buffer[0x10000];
            field_buffer[0] = 0;
            size_t num_field_chars = 0;
            if (type->variant == TYPE_STRUCT || type->variant == TYPE_UNION) {
                VariableLinkedListNode_t *field = type->compound.su_fields;
                while (field) {
                    num_field_chars += print_variable(field_buffer + num_field_chars, &field->value);
                    num_field_chars += sprintf(field_buffer + num_field_chars, "; ");
                    field = field->next;
                }
            }
            else {
                EnumFieldLinkedListNode_t *field = type->compound.e_fields;
                while (field) {
                    num_field_chars += sprintf(field_buffer + num_field_chars, "%.*s = %ld",
                        field->value.name.end - field->value.name.begin, field->value.name.begin, field->value.value);
                    if (field->next) {
                        num_field_chars += sprintf(field_buffer + num_field_chars, ", ");
                    }
                    else {
                        num_field_chars += sprintf(field_buffer + num_field_chars, " ");
                    }
                    field = field->next;
                }
            }

            if (type->compound.has_name) {
                num_chars += sprintf(buffer, "%s %s { %s}",
                    type_strs[type->variant], type->compound.name.begin, field_buffer);
            }
            else {
                num_chars += sprintf(buffer, "%s { %s}",
                    type_strs[type->variant], field_buffer);
            }
        }
        else if (type->compound.has_name) {
            num_chars += sprintf(buffer, "%s %s",
                type_strs[type->variant], type->compound.name.begin);
        }
    }
    return num_chars;
}