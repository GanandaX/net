//
// Created by Administrator on 2025/5/21.
//

#include "net.h"
#include "ex_msg.h"
#include "net_plat.h"
#include "pkt_buf.h"
#include "debug.h"
#include "netif.h"
#include "loop.h"
#include "ether.h"
#include "timer.h"
#include "arp.h"
#include "ipv4.h"
#include "sock.h"
#include "raw.h"

net_status_t net_init(void) {
    debug(DEBUG_INIT, "init net");
    net_plat_init();
    ex_msg_init();
    pkt_buf_init();
    netif_init();
    net_timer_init();
    loop_init();
    ether_init();
    arp_init();
    ipv4_init();
    socket_init();
    raw_init();

    return NET_OK;
}

net_status_t net_start(void) {
    ex_msg_start();

    debug(DEBUG_INIT, "net is running");

    return NET_OK;
}
