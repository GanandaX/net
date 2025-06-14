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

typedef struct _exmsg_t {

    n_list_node_t node;
    enum {
        NET_EXMSG_NETIF_IN,
    } type;

    union {
        msg_netif_t netif;
    };

} exmsg_t;

net_status_t ex_msg_init();

net_status_t ex_msg_start();

net_status_t exmsg_netif_in(netif_t *netif);

#endif //NET_EX_MSG_H
