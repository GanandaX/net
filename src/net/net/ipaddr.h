//
// Created by GrandBand on 2025/6/2.
//

#ifndef NET_IPADDR_H
#define NET_IPADDR_H

#include <stdint.h>
#include "net_status.h"

#define IPV4_ADDR_SIZE 4

typedef struct _ipaddr_t {
    enum {
        IPADDR_V4,
    } type;

    union {
        uint32_t q_addr;
        uint8_t a_addr[IPV4_ADDR_SIZE];
    };
} ipaddr_t;

void ipaddr_set_any(ipaddr_t *ip);

uint8_t ipaddr_is_any(ipaddr_t *ip);

net_status_t ipaddr_from_str(ipaddr_t *dest, const char *str);

void ipaddr_copy(ipaddr_t *dest, const ipaddr_t *src);

ipaddr_t *ipaddr_get_empty();

void ipaddr_from_buf(ipaddr_t *dest, uint8_t *buf);

void ipaddr_to_buf(const ipaddr_t *src, uint8_t *buf);

ipaddr_t ipaddr_get_net(const ipaddr_t *addr, const ipaddr_t *mask);

uint8_t ipaddr_is_local_broadcast(const ipaddr_t *ipaddr);

uint8_t ipaddr_is_direct_broadcast(const ipaddr_t *ipaddr, const ipaddr_t *mask);

uint8_t ipaddr_is_match(const ipaddr_t *dest, const ipaddr_t *src, const ipaddr_t *mask);

#endif //NET_IPADDR_H
