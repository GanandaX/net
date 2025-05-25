//
// Created by Administrator on 2025/5/21.
//

#ifndef NET_NET_STATUS_H
#define NET_NET_STATUS_H


typedef enum _net_status_t {
    NET_OK = 0,
    NET_ERROR = -1,
    NET_ERROR_MEM = -2,
    NET_ERROR_FULL = -3,
    NET_ERROR_TIMEOUT = -4,
} net_status_t;


#endif //NET_NET_STATUS_H
