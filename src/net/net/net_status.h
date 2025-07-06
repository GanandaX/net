//
// Created by Administrator on 2025/5/21.
//

#ifndef NET_NET_STATUS_H
#define NET_NET_STATUS_H


typedef enum _net_status_t {
    NET_ERROR_NEED_WAIT = 1,
    NET_OK = 0,
    NET_ERROR = -1,
    NET_ERROR_MEM = -2,
    NET_ERROR_FULL = -3,
    NET_ERROR_TIMEOUT = -4,
    NET_ERROR_SIZE = -5,
    NET_ERROR_NO_SOURCE = -6,
    NET_ERROR_PARAM = -7,
    NET_ERROR_STATE = -8,
    NET_ERROR_IO = -9,
    NET_ERROR_EXIST = -10,
    NET_ERROR_NOT_EXIST = -11,
    NET_ERROR_NOT_SUPPORT = -12,
    NET_ERROR_PACKAGE = -13,
    NET_ERROR_SYSTEM = -14,
    NET_ERROR_UNREACH = -15,
    NET_ERROR_CHECKSUM = -16,
} net_status_t;


#endif //NET_NET_STATUS_H
