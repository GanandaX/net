//
// Created by Administrator on 2025/5/21.
//
#include "net_plat.h"
#include "debug.h"

net_status_t net_plat_init() {
    debug(DEBUG_PLAT, "net_plat is initializing...");
    debug(DEBUG_PLAT, "init done.");

    return NET_OK;
}