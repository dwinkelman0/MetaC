#include "flow.h"

bool is_declared_in_scope(const AConstString_t type_name, const DataScope_t *const scope) {
    if (TypeNameMap_find(scope->types, type_name)) {
        return true;
    }
    else if (scope->location.parent_scope) {
        return is_declared_in_scope(type_name, scope->location.parent_scope);
    }
    return false;
}