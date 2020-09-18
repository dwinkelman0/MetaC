#ifndef _FLOW_FLOW_H_
#define _FLOW_FLOW_H_

#include "../grammar/grammar.h"

#include <string.h>

struct DataLocation;
struct DataVariable;
struct DataScope;
struct DataOperand;
struct DataOperation;
struct Function;
struct BasicBlock;

DEFINE_MAP(DataVariable, AConstString_t, struct DataVariable *, const AConstString_t, struct DataVariable *const, cmp_alloc_const_str)
DEFINE_MAP(TypeName, AConstString_t, DerivedType_t *, const AConstString_t, DerivedType_t *const, cmp_alloc_const_str)

typedef struct DataLocation {
    struct DataScope *parent_scope;
    uint64_t size;
    uint64_t alignment;
    uint64_t offset;
    bool has_offset;
} DataLocation_t;

typedef struct DataVariable {
    struct DataLocation *location;
    struct DataScope *scope;
    struct Function *function;
} DataVariable_t;

typedef struct DataScopeLinkedListNode {
    struct DataScope *value;
    struct DataScopeLinkedListNode *next;
} DataScopeLinkedListNode_t;

typedef struct DataScope {
    struct DataLocation location;
    struct DataVariableMapNode *variables;
    struct TypeNameMapNode *types;
    struct DataScopeLinkedListNode *scopes;
} DataScope_t;

typedef struct DataOperand {
    struct DataVariable *data;
    DerivedType_t *type;
} DataOperand_t;

typedef struct DataOperation {
    OperatorVariant_t op;
    struct DataOperand *in1;
    struct DataOperand *in2;
    struct DataOperand *out;
} DataOperation_t;

typedef struct DataOperationLinkedListNode {
    struct DataOperation *value;
    struct DataOperationLinkedListNode *next;
} DataOperationLinkedListNode_t;

typedef struct Function {
    struct BasicBlock *head_block;
    struct DataScope *scope;
    struct DataOperand *params;
} Function_t;

typedef struct BasicBlock {
    struct DataScope *scope;
    struct DataOperationLinkedListNode *ops;
    struct BasicBlock *next;
    struct BasicBlock *predicate;
    struct BasicBlock *branch;
    bool returns;
} BasicBlock_t;

/**
 * ERROR HANDLING
 */
typedef enum FlowErrorVariant {
    FLOW_ERROR,
    FLOW_WARNING
} FlowErrorVariant_t;
typedef struct FlowError {
    struct FlowError *next;
    FlowErrorVariant_t variant;
    AConstString_t cause;
    const char *desc;
} FlowError_t;

/**
 * FUNCTIONS
 */
void append_error(FlowError_t ***errors, const FlowErrorVariant_t variant, const AConstString_t cause, const char *const desc);

bool is_type_declared_in_scope(const AConstString_t type_name, const DataScope_t *const scope);
bool is_defined_in_scope(const AConstString_t type_name, const DataScope_t *const scope);

void flowify_statement(const Statement_t *statement, BasicBlock_t *block, DataScope_t *scope, FlowError_t ***errors);

#endif