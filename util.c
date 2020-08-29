#include "util.h"

#include <assert.h>
#include <string.h>

const char *strend(const char *str) {
    return str + strlen(str);
}

uint64_t parse_uint64_dec(const char *str) {
    return consume_uint64_dec(&str);
}

uint64_t consume_uint64_dec(const char **str) {
    uint64_t output = 0;
    const char **it = str;
    while (1) {
        if ('0' <= **it && **it <= '9') {
            output *= 10;
            output += **it - '0';
        }
        else {
            break;
        }
        ++(*it);
    }
    return output;
}

uint64_t parse_uint64_hex(const char *str) {
    return consume_uint64_hex(&str);
}

uint64_t consume_uint64_hex(const char **str) {
    uint64_t output = 0;
    assert((*str)[0] == '0' && (*str)[1] == 'x');
    const char **it = str;
    *it += 2;
    while (1) {
        if ('0' <= **it && **it <= '9') {
            output *= 16;
            output += **it - '0';
        }
        else if ('a' <= **it && **it <= 'f') {
            output *= 16;
            output += 10 + **it - 'a';
        }
        else if ('A' <= **it && **it <= 'F') {
            output *= 16;
            output += 10 + **it - 'A';
        }
        else {
            break;
        }
        ++(*it);
    }
    return output;
}

uint64_t parse_uint64(const char *str) {
    return consume_uint64(&str);
}

uint64_t consume_uint64(const char **str) {
    return ((*str)[0] == '0' && (*str)[1] == 'x') ?
        consume_uint64_hex(str) :
        consume_uint64_dec(str);
}