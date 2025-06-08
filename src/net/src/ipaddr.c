//
// Created by GrandBand on 2025/6/7.
//
#include "ipaddr.h"

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

    return (ipaddr_t *)&ipaddr_empty;
}