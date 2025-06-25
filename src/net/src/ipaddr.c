//
// Created by GrandBand on 2025/6/7.
//
#include "ipaddr.h"

#include "ether.h"

void ipaddr_set_any(ipaddr_t *ip) {
    ip->type = IPADDR_V4;
    ip->q_addr = 0;
}

net_status_t ipaddr_from_str(ipaddr_t *dest, const char *str) {
    if (!dest || !str) {
        return NET_ERROR_PARAM;
    }

    dest->type = IPADDR_V4;
    dest->q_addr = 0;

    uint8_t *p = dest->a_addr;
    char c;
    uint8_t value = 0;
    while ((c = *str++) != 0) {
        if (c <= '9' && c >= '0') {
            uint16_t verify_value = value * 10 + (c - '0');
            if (verify_value >= 256) {
                return NET_ERROR_PARAM;
            }
            value = verify_value;
        } else if (c == '.') {
            *p++ = value;
            value = 0;
        } else {
            return NET_ERROR_PARAM;
        }
    }

    *p = value;
    return NET_OK;
}

void ipaddr_copy(ipaddr_t *dest, const ipaddr_t *src) {
    if (!dest || !src) {
        return;
    }

    dest->type = src->type;
    dest->q_addr = src->q_addr;
}

ipaddr_t *ipaddr_get_empty() {
    static const ipaddr_t ipaddr_empty = {.type = IPADDR_V4, .q_addr = 0};

    return (ipaddr_t *) &ipaddr_empty;
}

void ipaddr_from_buf(ipaddr_t *dest, uint8_t *buf) {
    dest->type = IPADDR_V4;
    dest->q_addr = *(uint32_t *) buf;
}

void ipaddr_to_buf(const ipaddr_t *src, uint8_t *buf) {
    *(uint32_t *) buf = src->q_addr;
}

ipaddr_t ipaddr_get_net(const ipaddr_t *addr, const ipaddr_t *mask) {
    ipaddr_t result;

    result.q_addr = addr->q_addr & mask->q_addr;
    return result;
}

uint8_t ipaddr_is_local_broadcast(const ipaddr_t *ipaddr) {
    return ipaddr->q_addr == 0XFFFFFFFF;
}

uint8_t ipaddr_is_direct_broadcast(const ipaddr_t *ipaddr, const ipaddr_t *mask) {
    return ((ipaddr->q_addr & ~mask->q_addr) == ~mask->q_addr);
}

uint8_t ipaddr_is_match(const ipaddr_t *dest, const ipaddr_t *src, const ipaddr_t *mask) {
    if (ipaddr_is_local_broadcast(dest)) {
        return 1;
    }

    ipaddr_t dest_net =  ipaddr_get_net(dest, mask);
    ipaddr_t src_net =  ipaddr_get_net(src, mask);
    if (ipaddr_is_direct_broadcast(dest, mask) && ipaddr_is_equal(&dest_net, &src_net)) {
        return 1;
    }

    return ipaddr_is_equal(dest, src);
}
