//
// Created by GrandBand on 2025/6/11.
//

#ifndef NET_ETHER_H
#define NET_ETHER_H

#include "net_status.h"
#include "net_config.h"
#include "stdint.h"
#include "netif.h"
#include "protocol.h"

#define ETHER_MTU 1500
#define ETHER_DATA_MIN 46

#pragma pack(1)
typedef struct _ether_hdr_t {
    uint8_t dest[NETIF_HWADDR_SIZE];
    uint8_t src[NETIF_HWADDR_SIZE];
    uint16_t protocol;
} ether_hdr_t;

typedef struct _ether_pkt_t {
    ether_hdr_t hdr;
    uint8_t data[ETHER_MTU];
} ether_pkt_t;

#pragma pack()

net_status_t ether_init(void);


const uint8_t *ether_broadcast_addr(void);
net_status_t ether_raw_out(netif_t *netif, protocol_t protocol, const uint8_t *dest, pkt_buf_t *buf);

#endif //NET_ETHER_H
