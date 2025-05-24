//
// Created by Administrator on 2025/5/23.
//
#include "n_list.h"

void n_list_init(n_list_t *list) {
    debug_assert(list != (void *) 0, "不应为空");

    list->first = list->last = (void *) 0;
    list->count = 0;
}

void n_list_insert_first(n_list_t *list, n_list_node_t *node) {
    debug_assert(list != (void *) 0, "不应为空");
    debug_assert(node != (void *) 0, "不应为空");

    if (n_list_is_empty(list)) {
        node->next = (void *) 0;
        node->pre = (void *) 0;
        list->first = node;
        list->last = node;
    } else {
        node->next = list->first;
        node->pre = (void *) 0;
        list->first->pre = node;
        list->first = node;
    }
    list->count++;

}


void n_list_insert_last(n_list_t *list, n_list_node_t *node) {
    debug_assert(list != (void *) 0, "不应为空");
    debug_assert(node != (void *) 0, "不应为空");

    if (n_list_is_empty(list)) {
        node->next = (void *) 0;
        node->pre = (void *) 0;
        list->first = node;
        list->last = node;
    } else {
        node->next = (void *) 0;
        node->pre = list->last;
        list->last->next = node;
        list->last = node;
    }
    list->count++;

}

void n_list_travel(n_list_t *list, void(*fun)(n_list_node_t *)) {
    debug_assert(list != (void *) 0, "不应为空");
    debug_assert(fun != 0, "不应为空");

    n_list_node_t *node = list->first;
    while (node) {
        fun(node);
        node = node->next;
    }
}

unsigned char n_list_contain(n_list_t *list, n_list_node_t *node) {
    debug_assert(list != (void *) 0, "不应为空");
    debug_assert(node != (void *) 0, "不应为空");

    n_list_node_t *current_node = list->first;
    while (current_node) {
        if (current_node == node) {
            return 1;
        }
    }
    return 0;
}

n_list_node_t *n_list_remove(n_list_t *list, n_list_node_t *node) {
    debug_assert(list != (void *) 0, "不应为空");
    debug_assert(node != (void *) 0, "不应为空");


    if (list->first == node) {
        // 头节点
        list->first = node->next;
        if (list->count == 1) {
            list->last = (void *) 0;
        } else {
            node->next->pre = (void *) 0;
        }
    } else if (list->last == node) {
        // 尾节点
        list->last = node->pre;
        if (list->count == 1) {
            list->first = (void *) 0;
        } else {
            node->pre->next = (void *) 0;
        }
    } else {
        // 中间节点
        unsigned char is_contain = n_list_contain(list, node);

        if (!is_contain) {
            debug_warning(DEBUG_WARNING, "删除的节点非此链表内的");
            return (void *) 0;
        }

        node->next->pre = node->pre;
        node->pre->next = node->next;
    }

    list->count--;
    return node;
}


void n_list_insert_after(n_list_t *list, n_list_node_t *pre, n_list_node_t *node) {
    debug_assert(list != (void *) 0, "不应为空");
    debug_assert(node != (void *) 0, "不应为空");

    if ((pre == (void *) 0) || n_list_is_empty(list)) {
        // 插在头部或插入空链表
        n_list_insert_first(list, node);
    } else if (pre == list->last) {
        // 插在尾部
        n_list_insert_last(list, node);
    } else {
        node->pre = pre;
        node->next = pre->next;
        node->pre->next = node;
        node->next->pre = node;
    }

    list->count++;
}