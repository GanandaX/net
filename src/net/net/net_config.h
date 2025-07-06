//
// Created by Administrator on 2025/5/24.
//

#ifndef NET_CONFIG_H
#define NET_CONFIG_H

#define DEBUG_RAW DEBUG_ERROR
#define DEBUG_IP DEBUG_ERROR
#define DEBUG_ICMPV4 DEBUG_INFO
#define DEBUG_ARP DEBUG_INFO
#define DEBUG_TIMER DEBUG_INFO
#define DEBUG_ETHER DEBUG_INFO
#define DEBUG_NETIF DEBUG_INFO
#define DEBUG_INIT DEBUG_INFO
#define DEBUG_PLAT DEBUG_INFO
#define DEBUG_MSG DEBUG_INFO
#define DEBUG_BUF DEBUG_INFO
#define DEBUG_M_BLOCK DEBUG_ERROR
#define DEBUG_QUEUE DEBUG_ERROR
#define DEBUG_NO_FREE_BLCK DEBUG_ERROR

#define NET_ENDIAN_LITTLE       1

#define EXMSG_MSG_CNT           10
#define LOCKER_TYPE             N_LOCKER_THREAD
#define PKT_BLK_SIZE            128
#define PKT_BLK_CNT             128
#define PKT_BUF_CNT             128

#define NETIF_HWADDR_SIZE       6
#define NETIF_NAME_SIZE         10
#define NETIF_INQ_SIZE          50
#define NETIF_OUTQ_SIZE         50

#define NETIF_DEV_CNT           10

#define TIMER_NAME_SIZE         32

#define ARP_CACHE_SIZE          2

#define ARP_MAX_PKT_WAIT        5

#define ARP_TIMER_TMO           1      // s
#define ARP_ENTRY_STABLE_TMO    5
#define ARP_ENTRY_PENDING_TMO   3      // s
#define ARP_ENTRY_RETRY_CNT     5
#define IP_FRAGS_MAX_CNT        5
#define IP_FRAG_MAX_BUF_CNT     10

#define RAW_MAX_CNT             10

#endif //NET_CONFIG_H