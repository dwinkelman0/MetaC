#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "util.h"

#include <stddef.h>
#include <stdint.h>

#define TryGrammar(type) struct { bool success; union { type value; struct { const char *it; const char *desc; } error; }; }

struct Variable;
typedef struct Variable *VariablePtr;
typedef LinkedList(VariablePtr) VariableLinkedList_t;

struct EnumField;
typedef struct EnumField *EnumFieldPtr;
typedef LinkedList(EnumFieldPtr) EnumFieldLinkedList_t;

typedef enum {
    VOID, FLOAT, DOUBLE, CHAR, SHORT, INT, LONG, LONG_LONG,
    UNSIGNED_CHAR, UNSIGNED_SHORT, UNSIGNED_INT, UNSIGNED_LONG, UNSIGNED_LONG_LONG
} PrimitiveType_t;

typedef enum {
    PRIMITIVE_TYPE, STRUCT_TYPE, UNION_TYPE, ENUM_TYPE, NAMED_TYPE
} TypeVariant_t;

typedef struct Type {
    TypeVariant_t variant;
    union {
        PrimitiveType_t _primitive;
        struct {
            char *name;
            VariableLinkedList_t *fields;
            bool is_definition;
        } _struct;
        struct {
            char *name;
            VariableLinkedList_t *fields;
            bool is_definition;
        } _union;
        struct {
            char *name;
            EnumFieldLinkedList_t *fields;
            bool is_definition;
        } _enum;
        char *_named;
    };
} Type_t;

typedef enum {
    TERMINAL_TYPE,
    POINTER_TO_TYPE,
    ARRAY_OF_TYPE,
    FUNCTION_TYPE
} DerivedTypeVariant_t;

typedef enum {
    NONE,
    CONST,
    VOLATILE
} ConstVolatileQualification_t;

typedef struct DerivedType {
    DerivedTypeVariant_t variant;
    union {
        struct {
            ConstVolatileQualification_t qualification;
            Type_t *type;
        } terminal;
        struct {
            ConstVolatileQualification_t qualification;
            struct DerivedType *child_type;
        } pointer;
        struct {
            Optional(uint64_t) size;
            struct DerivedType *child_type;
        } array;
        struct {
            VariableLinkedList_t *params;
            struct DerivedType *return_type;
        } function;
    };
} DerivedType_t;

typedef struct Variable {
    char *name;
    DerivedType_t *type;
} Variable_t;

typedef TryGrammar(Variable_t) TryVariable_t;
TryVariable_t parse_variable(const ConstString *const input);
size_t print_variable(const Variable_t *const var, char *buffer);

#endif