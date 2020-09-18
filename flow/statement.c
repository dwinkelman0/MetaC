#include "flow.h"

#include <assert.h>

static bool try_declare_type(const Type_t *const type, DataScope_t *const scope, FlowError_t ***errors) {
    const Type_t *current = type;
    if (type->variant == TYPE_NAMED) {
        return TypeNameMap_find(scope->types, type->named);
    }
    else if (type->variant == TYPE_ENUM) {
        if (type->compound.has_name) {
            bool type_already_registered = is_declared_in_scope(type->compound.name, scope);
            if (type->compound.is_definition) {
                if (type_already_registered) {
                    append_error(errors,
                        FLOW_ERROR,
                        type->compound.name,
                        "Type is already defined");
                    return false;
                }
                DerivedType_t *der = malloc(sizeof(DerivedType_t));
                der->variant = DERIVED_TYPE_TERMINAL;
                der->terminal.qualifier = QUALIFIER_NONE;
                der->terminal.type = *type;
                TypeNameMap_insert(&scope->types, type->compound.name, der);
            }
            else {
                if (!type_already_registered) {
                    append_error(errors,
                        FLOW_ERROR,
                        type->compound.name,
                        "Type is not defined");
                    return false;
                }
                return true;
            }
        }
        if (type->compound.is_definition) {
            
        }
    }
    return true;
}

static bool try_declare_derived_type(const DerivedType_t *const der, DataScope_t *const scope, FlowError_t ***errors) {
    bool all_params_succeeded = true;
    const DerivedType_t *current = der;
    while (current) {
        if (current->variant == DERIVED_TYPE_POINTER) {
            current = current->pointer.inner_type;
        }
        else if (current->variant == DERIVED_TYPE_ARRAY) {
            current = current->array.inner_type;
        }
        else if (current->variant == DERIVED_TYPE_FUNCTION) {
            const VariableLinkedListNode_t *var = current->function.params;
            while (var) {
                all_params_succeeded = all_params_succeeded && try_declare_derived_type(var->value.type, scope, errors);
                var = var->next;
            }
            current = current->function.return_type;
        }
        else {
            return all_params_succeeded && try_declare_type(&current->terminal.type, scope, errors);
        }
    }
}

void flowify_statement(const Statement_t *statement, BasicBlock_t *block, DataScope_t *scope, FlowError_t ***errors) {
    switch (statement->variant) {
        case STATEMENT_DECLARATION:
            DataVariableMap_insert(&scope->variables, statement->declaration->name, statement->declaration->type);
            break;
        case STATEMENT_FUNCTION:
            statement->function->signature;
    }
}