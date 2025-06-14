//
// Created by Administrator on 2025/5/21.
//

#ifndef NET_NETIF_PCAP_H
#define NET_NETIF_PCAP_H
#include "net_status.h"
#include "stdint.h"
#include "netif.h"

typedef struct _pcap_data_t {
    const char *ip;
    const uint8_t *hwaddr;
}pcap_data_t;

extern const netif_ops_t net_dev_ops;
#endif //NET_NETIF_PCAP_H
