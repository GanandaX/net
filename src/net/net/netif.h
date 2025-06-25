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
#include "pkt_buf.h"

typedef struct _netif_hwaddr_t {
    uint8_t addr[NETIF_HWADDR_SIZE];
    uint8_t len;
} netif_hwaddr_t;

typedef enum _netif_type_t {
    NETIF_TYPE_NONE = 0,
    NETIF_TYPE_ETHER,
    NETIF_TYPE_LOOP,

    NETIF_TYPE_SIZE,
} netif_type_t;

struct _netif_t;

typedef struct _netif_ops_t {
    net_status_t (*open)(struct _netif_t *netif, void *data);

    void (*close)(struct _netif_t *netif);

    net_status_t (*xmit)(struct _netif_t *netif);
} netif_ops_t;


typedef struct _link_layer_t {
    netif_type_t type;

    net_status_t (*open)(struct _netif_t *netif);

    void (*close)(struct _netif_t *netif);

    net_status_t (*in)(struct _netif_t *netif, pkt_buf_t *buf);

    net_status_t (*out)(struct _netif_t *netif, ipaddr_t *dest, pkt_buf_t *buf);
} link_layer_t;

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

    netif_ops_t *ops;
    void *ops_data;
    const link_layer_t *link_layer;
    n_list_node_t node;
    fix_queue_t in_q;
    void *in_q_buf[NETIF_INQ_SIZE];
    fix_queue_t out_q;
    void *out_q_buf[NETIF_OUTQ_SIZE];
} netif_t;

net_status_t netif_init(void);

netif_t *netif_open(const char *dev_name, struct _netif_ops_t *ops, void *ops_data);

net_status_t netif_set_addr(netif_t *netif, ipaddr_t *ip, ipaddr_t *mask, ipaddr_t *gateway);

net_status_t netif_set_hwaddr(netif_t *netif, const char *hwaddr, int len);

net_status_t netif_set_active(netif_t *netif);

net_status_t netif_set_deactive(netif_t *netif);

net_status_t netif_close(netif_t *netif);

void netif_set_default(netif_t *netif);

netif_t *netif_get_default(void);

net_status_t netif_put_in(netif_t *netif, pkt_buf_t *buf, int tmo);

pkt_buf_t *netif_get_in(netif_t *netif, int tmo);

net_status_t netif_put_out(netif_t *netif, pkt_buf_t *buf, int tmo);

pkt_buf_t *netif_get_out(netif_t *netif, int tmo);

net_status_t netif_out(netif_t *netif, ipaddr_t *ipaddr, pkt_buf_t *buf);

net_status_t netif_register_layer(const link_layer_t *link_layer);

uint8_t hw_addr_is_equal(uint8_t *src_hwaddr, uint8_t *dest_hwaddr);

#endif //NET_NETIF_H
