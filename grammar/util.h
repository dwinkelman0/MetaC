#ifndef _GRAMMAR_UTIL_H_
#define _GRAMMAR_UTIL_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * STRING DATA STRUCTURE
 */
typedef struct {
    const char *begin;
    const char *end;
} ConstString_t;
ConstString_t const_string_from_cstr(const char *const str);
void print_const_string(const ConstString_t str, const char *message);

typedef struct {
    char *begin;
    char *end;
} AConstString_t;
AConstString_t new_alloc_const_string_from_cstr(const char *const str);
AConstString_t new_alloc_const_string_from_const_str(const ConstString_t str);
void free_alloc_const_string(AConstString_t *const str);

/**
 * TRY MACROS
 */
typedef enum {
    TRY_SUCCESS, // There is a valid result
    TRY_ERROR, // There was an error (with a message)
    TRY_NONE, // There was neither a valid result nor an error
} GrammarTryStatusVariant_t;
typedef struct Error {
    ConstString_t location;
    const char *desc;
} Error_t;
struct ErrorLinkedListNode;
#define GrammarTryType(type) \
    struct { \
        GrammarTryStatusVariant_t status; \
        union { \
            type value; \
            struct Error error; \
        }; \
    }
#define GrammarPropagateError(tried, output) \
    if (tried.status == TRY_ERROR) { \
        output.status = TRY_ERROR; \
        output.error.location = tried.error.location; \
        output.error.desc = tried.error.desc; \
        return output; \
    }

/**
 * FORWARD DECLARATIONS
 */
struct EnumField;
struct Type;
struct DerivedType;
struct Variable;
struct Expression;
struct Operator;
struct EnumFieldLinkedListNode;
struct VariableLinkedListNode;

struct Scope;
struct Statement;
struct Control;
struct GlobalContext;
struct GlobalDefinition;
struct StatementLinkedListNode;
struct GlobalDefinitionLinkedListNode;

/**
 * DATA TYPES
 */
typedef enum {
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_PRIMITIVE,
    TYPE_NAMED
} TypeVariant_t;
typedef enum {
    PRIMITIVE_VOID,
    PRIMITIVE_CHAR,
    PRIMITIVE_UNSIGNED_CHAR,
    PRIMITIVE_SHORT,
    PRIMITIVE_UNSIGNED_SHORT,
    PRIMITIVE_INT,
    PRIMITIVE_UNSIGNED_INT,
    PRIMITIVE_LONG,
    PRIMITIVE_UNSIGNED_LONG,
    PRIMITIVE_LONG_LONG,
    PRIMITIVE_UNSIGNED_LONG_LONG,
    PRIMITIVE_FLOAT,
    PRIMITIVE_DOUBLE,
} PrimitiveVariant_t;
typedef struct EnumField {
    AConstString_t name;
    int64_t value;
} EnumField_t;
typedef struct Type {
    TypeVariant_t variant;
    union {
        PrimitiveVariant_t primitive;
        struct {
            AConstString_t name;
            bool has_name;
            bool is_definition;
            union {
                struct VariableLinkedListNode *su_fields;
                struct EnumFieldLinkedListNode *e_fields;
            };
        } compound;
        AConstString_t named;
    };
    ConstString_t str;
} Type_t;

/**
 * DERIVED TYPES
 */
typedef enum {
    DERIVED_TYPE_TERMINAL,
    DERIVED_TYPE_POINTER,
    DERIVED_TYPE_ARRAY,
    DERIVED_TYPE_FUNCTION
} DerivedTypeVariant_t;
typedef enum {
    QUALIFIER_NONE,
    QUALIFIER_CONST,
    QUALIFIER_VOLATILE
} QualifierVariant_t;
typedef struct DerivedType {
    DerivedTypeVariant_t variant;
    union {
        struct {
            QualifierVariant_t qualifier;
            Type_t type;
        } terminal;
        struct {
            QualifierVariant_t qualifier;
            struct DerivedType *inner_type;
        } pointer;
        struct {
            struct DerivedType *inner_type;
            AConstString_t size;
            bool has_size;
        } array;
        struct {
            struct DerivedType *return_type;
            struct VariableLinkedListNode *params;
        } function;
    };
} DerivedType_t;

/**
 * VARIABLES
 */
typedef struct Variable {
    AConstString_t name;
    bool has_name;
    struct DerivedType *type;
} Variable_t;

/**
 * EXPRESSIONS
 */
typedef enum {
    EXPRESSION_OPERATOR,
    EXPRESSION_IDENTIFIER,
    EXPRESSION_TYPE,
    EXPRESSION_DECLARATION,
    EXPRESSION_STR_LIT,
    EXPRESSION_CHAR_LIT,
    EXPRESSION_UINT_LIT,
    EXPRESSION_VOID
} ExpressionVariant_t;
typedef struct Expression {
    ExpressionVariant_t variant;
    union {
        struct Operator *operator;
        AConstString_t identifier;
        struct DerivedType *type;
        struct Variable *decl;
        AConstString_t str_lit;
        AConstString_t char_lit;
        uint64_t uint_lit;
    };
} Expression_t;

/**
 * OPERATORS
 */
typedef enum {
    OP_COMMA,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN, OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN, OP_SL_ASSIGN, OP_SR_ASSIGN, OP_AND_ASSIGN, OP_XOR_ASSIGN, OP_OR_ASSIGN,
    OP_COND,
    OP_LOGICAL_OR,
    OP_LOGICAL_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_AND,
    OP_EQ, OP_NE,
    OP_GT, OP_LT, OP_GE, OP_LE,
    OP_SL, OP_SR,
    OP_ADD, OP_SUB,
    OP_MUL, OP_DIV, OP_MOD,
    OP_POS, OP_NEG, OP_LOGICAL_NOT, OP_BITWISE_NOT, OP_CAST, OP_DEREFERENCE, OP_ADDRESS, OP_SIZEOF,
    OP_CALL, OP_SUBSCRIPT, OP_MEM_ACCESS, OP_PTR_ACCESS
} OperatorVariant_t;
typedef struct Operator {
    OperatorVariant_t variant;
    uint8_t n_operands;
    union {
        struct Expression *uop;
        struct {
            struct Expression *lop;
            struct Expression *rop;
        };
        struct {
            struct Expression *pop;
            struct Expression *top;
            struct Expression *fop;
        };
    };
} Operator_t;

/**
 * INTERNAL SCOPE
 */
typedef struct Scope {
    struct StatementLinkedListNode *statements;
} Scope_t;
typedef enum {
    STATEMENT_SCOPE,
    STATEMENT_CONTROL,
    STATEMENT_OPERATOR,
    STATEMENT_DECLARATION
} StatementVariant_t;
typedef struct Statement {
    StatementVariant_t variant;
    ConstString_t str;
    union {
        struct Scope *scope;
        struct Control *control;
        struct Operator *operator;
        struct Variable *declaration;
    };
} Statement_t;
typedef enum {
    CONTROL_IF,
    CONTROL_WHILE,
    CONTROL_DO,
    CONTROL_FOR,
    CONTROL_BREAK,
    CONTROL_CONTINUE,
    CONTROL_RETURN
} ControlVariant_t;
typedef struct Control {
    ControlVariant_t variant;
    union {
        struct {
            struct Expression condition;
            struct Statement exec;
            union {
                struct {
                    struct Statement *continuation;
                } ctrl_if;
                struct {
                    struct Expression *init;
                    struct Expression *increment;
                } ctrl_for;
            };
        };
        struct Expression ret;
    };
} Control_t;

/**
 * GLOBAL SCOPE
 */
typedef enum {
    GLOBAL_DEFINITION_DECL,
    GLOBAL_DEFINITION_FUNC
} GlobalDefinitionVariant_t;
typedef struct GlobalContext {
    struct GlobalDefinitionLinkedListNode *definitions;
} GlobalContext_t;
typedef struct GlobalDefinition {
    GlobalDefinitionVariant_t variant;
    union {
        struct Operator declaration;
        struct {
            struct Variable declaration;
            struct Scope scope;
        } func;
    };
} GlobalDefinition_t;

/**
 * AUXILIARY DATA STRUCTURES
 */
typedef struct {
    ConstString_t str;
    int64_t integer;
} IntegerLiteral_t;
typedef GrammarTryType(IntegerLiteral_t) TryIntegerLiteral_t;

typedef GrammarTryType(ConstString_t) TryConstString_t;
typedef GrammarTryType(Type_t) TryType_t;
typedef GrammarTryType(Variable_t) TryVariable_t;
typedef GrammarTryType(EnumField_t) TryEnumField_t;
typedef GrammarTryType(Expression_t) TryExpression_t;
typedef GrammarTryType(Operator_t) TryOperator_t;
typedef GrammarTryType(char *) TryCharPtr_t;
typedef GrammarTryType(Scope_t) TryScope_t;
typedef GrammarTryType(Statement_t) TryStatement_t;

typedef struct ErrorLinkedListNode {
    struct ErrorLinkedListNode *next;
    struct Error value;
} ErrorLinkedListNode_t;
typedef struct EnumFieldLinkedListNode {
    struct EnumFieldLinkedListNode *next;
    struct EnumField value;
} EnumFieldLinkedListNode_t;
typedef struct VariableLinkedListNode {
    struct VariableLinkedListNode *next;
    struct Variable value;
} VariableLinkedListNode_t;
typedef struct StatementLinkedListNode {
    struct StatementLinkedListNode *next;
    struct Statement value;
} StatementLinkedListNode_t;

/**
 * Prefixes:
 *  - parse: accepts `const ConstString_t` input, returns `GrammarTryType`, return type is a value (not pointer)
 *  - find: accepts `const ConstString_t` inputs, returns `GrammarTryType(ConstString_t)`
 *  - is: accepts `const ConstString_t` input, returns `GrammarTryType(bool)`
 */

/**
 * Remove child from parent if child and parent share a boundary
 *  SUCCESS: share boundary condition is satisfied
 *  ERROR: share boundary condition is not satisfied
 */
TryConstString_t strip(const ConstString_t parent, const ConstString_t child);

/**
 * Find whitespace at the beginning of the string
 *  SUCCESS: there is at least one character of whitespace at the beginning
 *  NONE: there are no characters of whitespace at the beginning
 */
TryConstString_t find_whitespace(const ConstString_t str);

/**
 * Strip whitespace from the beginning of the string
 */
ConstString_t strip_whitespace(const ConstString_t str);

/**
 * If the string begins with the pattern, return the ConstString of the pattern
 * in the original string
 *  SUCCESS: the pattern was found
 *  NONE: the pattern was not found
 */
TryConstString_t find_string(const ConstString_t str, const ConstString_t pattern);

/**
 * Find the first instance of the pattern in the original string
 *  SUCCESS: the pattern was found
 *  NONE: the pattern was not found
 */
TryConstString_t find_first_string(const ConstString_t str, const ConstString_t pattern);

/**
 * If the string begins with an identifier, return the ConstString of the
 * identifier
 *  SUCCESS: an identifier was found
 *  NONE: an identifier was not found
 *  ERROR: the identifier is a keyword
 */
TryConstString_t find_identifier(const ConstString_t str);

/**
 * Check if the string is a keyword
 */
bool is_keyword(const ConstString_t str);

/**
 * If the string begins with an integer, return the ConstString and parsed
 * integer
 *  SUCCESS: an integer was found
 *  NONE: an integer was not found
 */
TryIntegerLiteral_t find_integer(const ConstString_t str);

/**
 * Find the closing token, return the string enclosing the tokens
 *  SUCCESS: the first and last characters of the enclosed string are valid
 *  NONE: the first character is not valid
 *  ERROR: there is not a closing token
 */
TryConstString_t find_closing(const ConstString_t str, const char opening, const char closing);

/**
 * Capture the contents of a quote-enclosed string literal; output includes the
 * quotes
 *  SUCCESS: there are starting and opening quotes and a string was captured
 *  NONE: there was no starting quote
 *  ERROR: there was no closing quote
 */
TryConstString_t find_string_lit(const ConstString_t str, const char quote, const char escape);

/**
 * Find the next location of the pattern in the string that is in the same
 * level of parentheses/braces/brackets/strings/chars as the beginning of the
 * string
 *  SUCCESS: a string was found satisfying the requirements
 *  NONE: no string was found satisfying the requirements
 *  ERROR: propagating error from closure detection
 */
TryConstString_t find_string_nesting_sensitive(const ConstString_t str, const ConstString_t pattern);

/**
 * Find the last location of the pattern in the string that is in the same
 * level of parentheses/braces/brackets/strings/chars as the beginning of the
 * string
 *  SUCCESS: a string was found satisfying the requirements
 *  NONE: no string was found satisfying the requirements
 *  ERROR: propagating error from closure detection
 */
TryConstString_t find_last_string_nesting_sensitive(const ConstString_t str, const ConstString_t pattern);

/**
 * Find the last location of the pattern in the string that is in the same
 * level of parentheses/braces/brackets/strings/chars as the beginning of the
 * string
 *  SUCCESS: a string was found satisfying the requirements
 *  NONE: no string was found satisfying the requirements
 *  ERROR: propagating error from closure detection
 */
TryConstString_t find_last_closure_nesting_sensitive(const ConstString_t str, const char opening, const char closing);

#endif