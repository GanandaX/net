//
// Created by GrandBand on 2025/6/15.
//

#ifndef NET_ARP_H
#define NET_ARP_H

#include "ipaddr.h"
#include "ether.h"


#define ARP_HW_ETHER    1
#define ARP_REQUEST     1
#define ARP_REPLY       2

#pragma pack(1)
typedef struct _arp_pkt_t {

    uint16_t htype;
    uint16_t ptype;
    uint8_t hwlen;
    uint8_t plen;
    uint16_t opcode;
    uint8_t sender_hwaddr[NETIF_HWADDR_SIZE];
    uint8_t sender_paddr[IPV4_ADDR_SIZE];
    uint8_t target_hwaddr[NETIF_HWADDR_SIZE];
    uint8_t target_paddr[IPV4_ADDR_SIZE];
} arp_pkt_t;
#pragma pack()

typedef struct _arp_entry_t {
    // 目的ip和端口
    uint8_t paddr[IPV4_ADDR_SIZE];
    uint8_t hwaddr[NETIF_HWADDR_SIZE];

    enum {
        NET_ARP_FREE,
        NET_ARP_WAITING,
        NET_ARP_RESOLVED,
    } state;

    uint32_t tmo;
    uint32_t retry;

    n_list_node_t node;
    n_list_t buf_list;

    netif_t *netif;
} arp_entry_t;

net_status_t arp_init();

net_status_t arp_make_request(netif_t *netif, const ipaddr_t *ip);

net_status_t arp_make_reply(netif_t *netif, pkt_buf_t *buf);

net_status_t arp_make_gratuitous(netif_t *netif);

net_status_t arp_in(netif_t *netif, pkt_buf_t *buf);

net_status_t arp_resolve(netif_t *netif, ipaddr_t *ipaddr, pkt_buf_t *buf);

void arp_clear(netif_t *netif);

const uint8_t *arp_find(netif_t *netif, ipaddr_t *ipaddr);

uint8_t ipaddr_is_local_broadcast(const ipaddr_t *ipaddr);

uint8_t ipaddr_is_direct_broadcast(const ipaddr_t *ipaddr, const ipaddr_t *mask);

#endif //NET_ARP_H
