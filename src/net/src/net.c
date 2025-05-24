//
// Created by Administrator on 2025/5/21.
//

#include "net.h"
#include "ex_msg.h"
#include "net_plat.h"

net_status_t net_init(void) {

    net_plat_init();

    ex_msg_init();
    return NET_OK;
}

net_status_t net_start(void) {
    ex_msg_start();
    return NET_OK;
}