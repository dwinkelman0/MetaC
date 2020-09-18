#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>

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

#define DEFINE_MAP(name, ktype, vtype, const_ktype, const_vtype, kcmp) \
    typedef struct name ## MapNode { \
        ktype key; \
        vtype value; \
        struct name ## MapNode *left; \
        struct name ## MapNode *right; \
    } name ## MapNode_t; \
    const name ## MapNode_t **name ## Map_insert(name ## MapNode_t **const root, const_ktype key, const_vtype value) { \
        name ## MapNode_t **head = root; \
        while (head) { \
            int cmp_result = kcmp((*head)->key, key); \
            if (cmp_result == 0) return NULL; \
            head = cmp_result < 0 ? &(*head)->left : &(*head)->right; \
        } \
        *head = malloc(sizeof(name ## MapNode_t)); \
        (*head)->key = key; \
        (*head)->value = value; \
        (*head)->left = NULL; \
        (*head)->right = NULL; \
        return head; \
    } \
    const name ## MapNode_t **name ## Map_find(const name ## MapNode_t *const root, const_ktype key) { \
        name ## MapNode_t **head = &root; \
        while (head) { \
            int cmp_result = kcmp((*head)->key, key); \
            if (cmp_result == 0) return head; \
            head = cmp_result < 0 ? &(*head)->left : &(*head)->right; \
        } \
        return NULL; \
    } \
    void name ## Map_foreach(const name ## MapNode_t *const root, void (*func)(const name ## MapNode_t *node, void *args), void *args) { \
        if (!root) return; \
        name ## Map_foreach(root->left, func, args); \
        func(root, args); \
        name ## Map_foreach(root->right, func, args); \
    } \
    void name ## Map_free(name ## MapNode_t **const root) { \
        if (!*root) return; \
        name ## Map_free((*root)->left); \
        name ## Map_free((*root)->right); \
        free(*root); \
        *root = NULL; \
    }

#endif