//
// Created by Administrator on 2025/5/21.
//

#include "net.h"
#include "ex_msg.h"
#include "net_plat.h"
#include "pkt_buf.h"
#include "debug.h"

net_status_t net_init(void) {
    debug(DEBUG_INIT, "init net");
    net_plat_init();
    ex_msg_init();
    pkt_buf_init();
    return NET_OK;
}

net_status_t net_start(void) {
    ex_msg_start();

    debug(DEBUG_INIT, "net is running");

    return NET_OK;
}
