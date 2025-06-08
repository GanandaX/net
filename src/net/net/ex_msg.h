//
// Created by Administrator on 2025/5/21.
//

#ifndef NET_EX_MSG_H
#define NET_EX_MSG_H

#include "net_status.h"
#include "n_list.h"
#include "netif.h"

typedef struct _exmsg_t {
    enum {
        NET_EXMSG_NETIF_IN
    } type;

    int id;
} exmsg_t;

net_status_t ex_msg_init();

net_status_t ex_msg_start();

net_status_t exmsg_netif_in(netif_t *netif);

#endif //NET_EX_MSG_H
