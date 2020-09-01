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

struct Operator;
typedef enum {
    OPERATOR_OPERAND,
    IDENTIFIER_OPERAND,
    TYPE_OPERAND,
    STR_LIT_OPERAND,
    SIGNED_INT_LIT_OPERAND,
    UNSIGNED_INT_LIT_OPERAND,
    FLOAT_LIT_OPERAND,
    DOUBLE_LIT_OPERAND
} OperandVariant_t;
typedef enum {
    OP_COMMA,
    OP_ASSIGN,
    OP_COND,
    OP_LOGICAL_AND,
    OP_LOGICAL_OR,
    OP_BITWISE_AND,
    OP_BITWISE_XOR,
    OP_BITWISE_OR,
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GE, OP_LE,
    OP_SL, OP_SR,
    OP_ADD, OP_SUB,
    OP_MUL, OP_DIV, OP_MOD,
    OP_PREFIX_INC, OP_PREFIX_DEC, OP_POS, OP_NEG, OP_LOGICAL_NOT, OP_BITWISE_NOT, OP_CAST, OP_DEREFERENCE, OP_ADDRESS, OP_SIZEOF,
    OP_POSTFIX_INC, OP_POSTFIX_DEC, OP_FUNCTION_CALL, OP_ARRAY_SUBSCRIPT, OP_MEMBER_ACCESS, OP_POINTER_MEMBER_ACCESS
} OperatorVariant_t;

typedef struct Operator {
    OperatorVariant_t variant;
    uint32_t n_operands;
    struct {
        OperandVariant_t variant;
        union {
            struct Operator *op;
            char *identifier;
            Type_t *type;
            int64_t int_lit;
            uint64_t uint_lit;
            char *str_lit;
            float float_lit;
            double double_lit;
        };
    } operands[3];
} Operator_t;

typedef TryGrammar(Variable_t) TryVariable_t;
TryVariable_t parse_variable(const ConstString *const input);
size_t print_variable(const Variable_t *const var, char *buffer);

typedef TryGrammar(Operator_t) TryOperator_t;
TryOperator_t parse_operator(const ConstString *const input);
size_t print_operator(const Operator_t *const op, char *buffer);

#endif