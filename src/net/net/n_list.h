//
// Created by Administrator on 2025/5/23.
//

#ifndef NET_N_LIST_H
#define NET_N_LIST_H

#include "debug.h"


typedef struct _n_list_node_t {
    struct _n_list_node_t *pre;
    struct _n_list_node_t *next;
} n_list_node_t;


static inline void n_list_node_init(n_list_node_t *node) {
    debug_assert(node != (n_list_node_t *) 0, "不应为空");
    node->next = node->pre = (n_list_node_t *) 0;
}

static inline n_list_node_t *n_list_node_pre(n_list_node_t *node) {
    debug_assert(node != (n_list_node_t *) 0, "不应为空");
    return node->pre;
}

static inline n_list_node_t *n_list_node_next(n_list_node_t *node) {
    debug_assert(node != (n_list_node_t *) 0, "不应为空");
    return node->next;
}

/*
 * 将next节点添加为node节点的下一个
 */
static inline void n_list_node_set_next(n_list_node_t *node, n_list_node_t *next) {
    debug_assert(node != (n_list_node_t *) 0, "不应为空");
    debug_assert(next != (n_list_node_t *) 0, "不应为空");

    next->next = node->next;
    next->pre = node;
    if (node->next) {
        node->next->pre = next;
    }
    node->next = next;
}

/*
 * 将pre节点添加为node节点的前一个
 */
static inline void n_list_node_set_pre(n_list_node_t *node, n_list_node_t *pre) {
    debug_assert(node != (n_list_node_t *) 0, "不应为空");
    debug_assert(pre != (n_list_node_t *) 0, "不应为空");

    pre->pre = node->pre;
    pre->next = node;
    if (node->pre) {
        node->pre->next = pre;
    }
    node->pre = pre;
}

typedef struct _n_list_t {
    n_list_node_t *first;
    n_list_node_t *last;
    int count;
} n_list_t;

void n_list_init(n_list_t *list);

static inline n_list_node_t *n_list_first(n_list_t *list) {
    debug_assert(list != (n_list_t *) 0, "不应为空");
    return list->first;
}

static inline n_list_node_t *n_list_last(n_list_t *list) {
    debug_assert(list != (n_list_t *) 0, "不应为空");
    return list->last;
}

static inline int n_list_is_empty(n_list_t *list) {
    debug_assert(list != (n_list_t *) 0, "不应为空");
    return list->count == 0;
}


static inline int n_list_count(n_list_t *list) {
    debug_assert(list != (n_list_t *) 0, "不应为空");
    return list->count;
}

/**
 * 在链表头部插入
 * @param list 
 * @param node 
 */
void n_list_insert_first(n_list_t *list, n_list_node_t *node);

void n_list_insert_last(n_list_t *list, n_list_node_t *node);

void n_list_travel(n_list_t *list, void (*fun)(n_list_node_t *node));

unsigned char n_list_contain(n_list_t *list, n_list_node_t *node);

#define n_list_for_each(node, list) for((node) = (list)->first; (node); (node) = (node) -> next)

#define n_offset_in_parent(parameter_name, parent_type) \
    (char *)(&(((parent_type*)0))->parameter_name)

#define n_offset_to_parent(node, parameter_name, parent_type) \
    (parent_type *)((char *)(node) - n_offset_in_parent(parameter_name, parent_type))

//#define n_list_entity(node, parameter_name, parent_type) \
//    (parent_type *)((node) ? (parent_type *)((char *)(node) - (char *)(&(((parent_type*)0))->parameter_name)) : 0)
#define n_list_entity(node, parameter_name, parent_type) \
    (parent_type *)((node) ? n_offset_to_parent(node, parameter_name, parent_type) : (void *)0)

n_list_node_t *n_list_remove(n_list_t *list, n_list_node_t *node);

static inline n_list_node_t *n_list_remove_first(n_list_t *list) {
    if (list->first) {
        return n_list_remove(list, list->first);
    }
    return (void *) 0;
}

static inline n_list_node_t *n_list_remove_last(n_list_t *list) {
    if (list->last) {
        return n_list_remove(list, list->last);
    }
    return (void *) 0;
}

void n_list_insert_after(n_list_t *list, n_list_node_t *pre, n_list_node_t *node);

#endif //NET_N_LIST_H
