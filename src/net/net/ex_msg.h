//
// Created by Administrator on 2025/5/21.
//

#ifndef NET_EX_MSG_H
#define NET_EX_MSG_H

#include "net_status.h"
#include "n_list.h"
#include "netif.h"

typedef struct _msg_netif_t {
    netif_t *netif;
}msg_netif_t;

struct _func_msg_t;
typedef net_status_t (*exmsg_func_t)(struct _func_msg_t *msg);

typedef struct _func_msg_t {
    sys_thread_t thread;

    exmsg_func_t func;
    void *param;
    net_status_t status;

    sys_sem_t wait_sem;
}func_msg_t;

typedef struct _exmsg_t {

    n_list_node_t node;
    enum {
        NET_EXMSG_NETIF_IN,
        NET_EXMSG_FUN,
    } type;

    union {
        msg_netif_t netif;
        func_msg_t *func;
    };

} exmsg_t;

net_status_t ex_msg_init();

net_status_t ex_msg_start();

net_status_t exmsg_netif_in(netif_t *netif);

net_status_t exmsg_func_exec(exmsg_func_t func, void *param);

#endif //NET_EX_MSG_H
