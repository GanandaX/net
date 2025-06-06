//
// Created by Administrator on 2025/6/6.
//


#include "netif.h"
#include "m_block.h"

static netif_t netif_buffer[NETIF_DEV_CNT];
static m_block_t netif_block;
static n_list_t netif_list;

net_status_t netif_init(void) {

    debug(DEBUG_INFO, "netif init");


    return NET_OK;
}
