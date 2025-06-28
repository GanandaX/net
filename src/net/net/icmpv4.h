//
// Created by Administrator on 2025/6/25.
//

#ifndef ICMPV4_H
#define ICMPV4_H

#include "net_status.h"
#include "ipaddr.h"
#include "pkt_buf.h"


typedef enum _icmp_type_t {
    ICMPV4_ECHO_REQUEST = 8,
    ICMPV4_ECHO_REPLY = 0,
    ICMPV4_UNREACHABLE = 3,
}icmp_type_t;

typedef enum _icmp_code_t {
    ICMPV4_ECHO = 0,
    ICMPV4_UNREACH_PORT = 3,
}icmp_code_t;

#pragma pack(1)
typedef struct _icmpv4_hdr_t {

    uint8_t type;
    uint8_t code;
    uint16_t checksum;
}icmpv4_hdr_t;

typedef struct _icmpv4_pkt_t {

    icmpv4_hdr_t hdr;
    union {
        uint32_t reserve;
    };
    uint8_t data[1];
}icmpv4_pkt_t;

#pragma pack()

net_status_t icmpv4_init(void);
net_status_t icmpv4_in(ipaddr_t *src, ipaddr_t *dst, pkt_buf_t *buf);
net_status_t icmpv4_out_unreach(ipaddr_t *dest_ip, ipaddr_t *src_ip, uint8_t code, pkt_buf_t *ip_buf);
#endif //ICMP_H