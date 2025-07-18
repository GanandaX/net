//
// Created by Administrator on 2025/6/28.
//

#ifndef PING_H
#define PING_H

#define PING_BUFFER_SIZE        4096
#define IP_ADDR_SIZE            4
#define PING_DEFAULT_ID         0X300

#include <stdint.h>

#pragma pack(1)
typedef struct _ip_header_t {
    uint16_t shdr: 4;
    uint16_t version: 4;
    uint16_t tos: 8;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_all;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t hdr_checksum;
    uint8_t src_ip[IP_ADDR_SIZE];
    uint8_t dest_ip[IP_ADDR_SIZE];
} ip_header_t;

typedef struct _icmp_hdr_t {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_hdr_t;

#pragma pack()

typedef struct _echo_req_t {
    icmp_hdr_t echo_hdr;
    char buf[PING_BUFFER_SIZE];
} echo_req_t;

typedef struct _echo_reply_t {
    ip_header_t ip_hdr;
    icmp_hdr_t echo_hdr;
    char buf[PING_BUFFER_SIZE];
} echo_reply_t;

typedef struct _ping_t {
    echo_req_t req;
    echo_reply_t reply;
} ping_t;

void ping_run(ping_t *ping, const char *dest, uint16_t times, uint32_t size, uint16_t interval);

#endif //PING_H
