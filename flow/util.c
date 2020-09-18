#include "flow.h"

void append_error(FlowError_t ***errors, const FlowErrorVariant_t variant, const AConstString_t cause, const char *const desc) {
    FlowError_t *new_error = malloc(sizeof(FlowError_t));
    new_error->next = NULL;
    new_error->variant = FLOW_ERROR;
    new_error->cause = new_alloc_const_string_from_alloc_const_str(cause);
    new_error->desc = desc;
    assert(!**errors);
    (**errors)->next = new_error;
    *errors = &new_error->next;
}