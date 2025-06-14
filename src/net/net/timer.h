//
// Created by GrandBand on 2025/6/14.
//

#ifndef NET_TIMER_H
#define NET_TIMER_H


#include "net_config.h"
#include "net_status.h"
#include "n_list.h"
#include "stdint.h"

#define NET_TIMER_RELOAD    (1 << 0)
#define NET_TIMER_ADDED     (1 << 1)

struct _net_timer_t;

typedef void(*timer_proc_t)(struct _net_timer_t *timer, void *arg);

typedef struct _net_timer_t {
    char name[TIMER_NAME_SIZE];
    uint32_t flags;

    uint32_t counter;           // 倒计时毫秒数
    uint32_t reload;

    timer_proc_t proc;
    void *arg;

    n_list_node_t node;
} net_timer_t;

net_status_t net_timer_init(void);

net_status_t
net_timer_add(net_timer_t *timer, const char *name, timer_proc_t proc, void *arg, uint32_t ms, uint32_t flags);

net_status_t net_timer_remove(net_timer_t *timer);

net_status_t net_timer_check_tmo(int interval);

uint32_t net_timer_first_tmo(void);
#endif //NET_TIMER_H
