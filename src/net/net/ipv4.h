//
// Created by Administrator on 2025/6/20.
//

#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

#include "netif.h"
#include "net_status.h"
#include "net_config.h"

#define IPV4_ADDR_SIZE              4
#define NET_VERSION_IPV4            4
#define NET_IP_DEFAULT_TTL          64

#pragma pack(1)
typedef struct _ipv4_header_t {
    union {
        struct {
#if NET_ENDIAN_LITTLE
            uint16_t shdr: 4;
            uint16_t version: 4;
            uint16_t tos: 8;
#else
            uint16_t version: 4;
            uint16_t shdr: 4;
            uint16_t tos: 8;
#endif
        };

        uint16_t shdr_all;
    };

    uint16_t total_len;
    uint16_t id;

    union {
        struct {
#if NET_ENDIAN_LITTLE
            uint16_t frag_offset: 13;
            uint16_t more: 1;
            uint16_t disable: 1;
            uint16_t reserved: 1;

#else
            uint16_t reserved: 1;
            uint16_t disable: 1;
            uint16_t more: 1;
            uint16_t frag_offset: 13;
#endif
        };

        uint16_t frag_all;
    };


    uint8_t ttl;
    uint8_t protocol;
    uint16_t hdr_checksum;
    uint8_t src_ip[IPV4_ADDR_SIZE];
    uint8_t dest_ip[IPV4_ADDR_SIZE];
} ipv4_header_t;

typedef struct _ipv4_pkt_t {
    ipv4_header_t hdr;
    uint8_t data[1];
} ipv4_pkt_t;

#pragma pack()


typedef struct _ip_frag_t {
    ipaddr_t ip;
    uint16_t id;
    uint32_t tmo;
    n_list_t buf_list;
    n_list_node_t node;
} ip_frag_t;


net_status_t ipv4_init(void);

net_status_t ipv4_in(netif_t *netif, pkt_buf_t *buf);

net_status_t ipv4_out(uint8_t protocol, ipaddr_t *dest, ipaddr_t *src, pkt_buf_t *buf);

static inline uint8_t ipv4_hdr_size(ipv4_pkt_t *pkt) {
    return pkt->hdr.shdr * 4;
}

static inline void ipv4_set_hdr_size(ipv4_pkt_t *pkt, int len) {
    pkt->hdr.shdr = len / 4;
}

#endif //IPV4_H
