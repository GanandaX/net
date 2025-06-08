//
// Created by GrandBand on 2025/6/7.
//
#include "loop.h"
#include "ex_msg.h"

static net_status_t loop_open(netif_t *netif, void *data) {

    netif->type = NETIF_TYPE_LOOP;
    return NET_OK;
}

static void loop_close(netif_t *netif) {

}


static net_status_t loop_xmit(netif_t *netif) {

    pkt_buf_t *pkt_buf = netif_get_out(netif, -1);
    if (pkt_buf) {
        net_status_t status = netif_put_in(netif, pkt_buf, -1);
        if (status < NET_OK) {
            pkt_buf_free(pkt_buf);
            return status;
        }
    }

    return NET_OK;
}

static netif_ops_t loop_ops = {
        .open = loop_open,
        .close = loop_close,
        .xmit = loop_xmit,
};

net_status_t loop_init(void) {
    debug(DEBUG_INFO, "init loop");

    netif_t *netif = netif_open("loop", &loop_ops, 0);
    if (!netif) {
        debug(DEBUG_ERROR, "open loop error");
        return NET_ERROR_NO_SOURCE;
    }

    ipaddr_t ip, mask;

    if (ipaddr_from_str(&ip, "127.0.0.1") < NET_OK) {
        return NET_ERROR_PARAM;
    }
    if (ipaddr_from_str(&mask, "255.255.255.0") < NET_OK) {
        return NET_ERROR_PARAM;
    }

    netif_set_addr(netif, &ip, &mask, (ipaddr_t *) 0);

    netif_set_active(netif);

    pkt_buf_t  *buf = pkt_buf_alloc(100);
    netif_out(netif, (ipaddr_t *)0, buf);

    debug(DEBUG_INFO, "init done.");
    return NET_OK;
}