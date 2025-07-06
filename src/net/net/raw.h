//
// Created by Administrator on 2025/6/30.
//

#ifndef RAW_H
#define RAW_H

#include "sock.h"


typedef struct _raw_t {
    sock_t base;

    n_list_t recv_list;
    sock_wait_t recv_wait;
} raw_t;

net_status_t raw_init(void);

sock_t *raw_create(int family, int protocol);

net_status_t raw_in(pkt_buf_t *pkt_buf);

#endif //RAW_H
