#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

typedef enum { false, true } bool;

#define Optional(type) struct { type value; bool has_value; }
#define LinkedList(type) struct LinkedList_##type { type value; struct LinkedList_##type *next; }

const char *strend(const char *str);

uint64_t parse_uint64_dec(const char *str);
uint64_t consume_uint64_dec(const char **str);
uint64_t parse_uint64_hex(const char *str);
uint64_t consume_uint64_hex(const char **str);
uint64_t parse_uint64(const char *str);
uint64_t consume_uint64(const char **str);



#endif