#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "util.h"

#include <stdint.h>

#define TryGrammar(type) struct { bool success; union { type value; struct { const char *it; const char *desc; } error; }; }

struct Variable;
typedef struct Variable *VariablePtr;
typedef LinkedList(VariablePtr) VariableLinkedList_t;

typedef enum {
    VOID, FLOAT, DOUBLE, CHAR, SHORT, INT, LONG, LONG_LONG
} PrimitiveType_t;

typedef enum {
    PRIMITIVE,
    STRUCT,
    UNION,
    ENUM,
    OTHER_NAME
} NamedType_t;

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
            NamedType_t variant;
            union {
                PrimitiveType_t primitive;
                char *name;
            };
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
TryVariable_t parse_variable(const char *const begin, const char *const end);
void print_variable(const Variable_t *const var, char *buffer);

#endif