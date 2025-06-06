//
// Created by GrandBand on 2025/6/2.
//

#ifndef NET_NETIF_H
#define NET_NETIF_H

#include <stdint.h>
#include "ipaddr.h"
#include "n_list.h"
#include "fix_queue.h"
#include "net_config.h"
#include "net_status.h"

typedef struct _netif_hwaddr_t {
    uint8_t addr[NETIF_HWADDR_SIZE];
    uint8_t len;

} netif_hwaddr_t;

typedef enum _netif_type_t {
    NETIF_TYPE_NONE = 0,
    NETIF_TYPE_ETHER,
    NETIF_TYPE_LOOP,

    NET_TYPE_SIZE,
} netif_type_t;

typedef struct _netif_t {
    char name[NETIF_NAME_SIZE];

    netif_hwaddr_t hwaddr;

    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t gateway;

    netif_type_t type;
    int mtu;

    enum {
        NETIF_CLOSED,
        NETIF_OPENED,
        NETIF_ACTIVE,
    } state;

    n_list_node_t node;
    fix_queue_t in_q;
    void *in_q_buf[NETIF_INQ_SIZE];
    fix_queue_t out_q;
    void *out_q_buf[NETIF_OUTQ_SIZE];


} netif_t;

net_status_t netif_init(void);

#endif //NET_NETIF_H
