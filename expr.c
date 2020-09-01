#include "grammar.h"

TryOperator_t parse_operator(const ConstString *const input) {
    TryOperator_t output;
    output.success = true;
    output.value.n_operands = 0;
}