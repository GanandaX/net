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
    NET_ERROR_SIZE = -5,
    NET_ERROR_NO_SOURCE = -6,
    NET_ERROR_PARAM = -7,
} net_status_t;


#endif //NET_NET_STATUS_H
