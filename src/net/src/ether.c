//
// Created by GrandBand on 2025/6/11.
//
#include "ether.h"
#include "pkt_buf.h"
#include "netif.h"
#include "debug.h"
#include "pkt_buf.h"
#include "tools.h"
#include "protocol.h"
#include "arp.h"

#if DEBUG_DISP_ENABLED(DEBUG_ETHER)
static void display_ether_pkt(char *title, ether_pkt_t *pkt, int total_size) {
    ether_hdr_t *hdr = &pkt->hdr;

    plat_printf("----------%s----------\n", title);
    plat_printf("len: %d %-10s", total_size, "bytes");
    dbg_dump_hwaddr("dest: ", hdr->dest, NETIF_HWADDR_SIZE);
    dbg_dump_hwaddr("src: ", hdr->src, NETIF_HWADDR_SIZE);
    plat_printf("\ttype: %04x", x_ntohs(hdr->protocol));


    switch (x_ntohs(hdr->protocol)) {
        case NET_PROTOCOL_ARP:
            plat_printf("%10s\n", "ARP");
            break;
        case NET_PROTOCOL_IPv4:
            plat_printf("%10s\n", "IPv4");
            break;

        default:
            plat_printf("%10s\n", "unknow");
            break;
    }
}
#else
#define display_ether_pkt(title, pkt, total_size)
#endif

net_status_t ether_open(struct _netif_t *netif) {
    return arp_make_gratuitous(netif);
}

void ether_close(struct _netif_t *netif) {
    arp_clear(netif);
}

static net_status_t is_pkt_ok(ether_pkt_t *frame, int total_size) {

    if (total_size > sizeof(ether_hdr_t) + ETHER_MTU) {
        debug(DEBUG_WARNING, "frame size to big: %d", total_size);
        return NET_ERROR_SIZE;
    }

    if (total_size < sizeof(ether_hdr_t)) {
        debug(DEBUG_WARNING, "frame size to small: %d", total_size);
        return NET_ERROR_SIZE;
    }

    return NET_OK;
}

net_status_t ether_in(struct _netif_t *netif, pkt_buf_t *buf) {

    debug(DEBUG_INFO, "ether in");

    pkt_buf_set_continue_space(buf, sizeof(ether_hdr_t));
    ether_pkt_t *pkt = (ether_pkt_t *) pkt_buf_data(buf);

    net_status_t status;
    if ((status = is_pkt_ok(pkt, buf->total_size)) < NET_OK) {
        debug(DEBUG_WARNING, "ether pkt error");
        return status;
    }

    display_ether_pkt("ether in", pkt, buf->total_size);

    switch (x_ntohs(pkt->hdr.protocol)) {
        case NET_PROTOCOL_ARP:
            status = pkt_remove_header(buf, sizeof(ether_hdr_t));
            if (status < NET_OK) {
                debug(DEBUG_ERROR, "remove header failed.");
                return status;
            }
            return arp_in(netif, buf);
            break;

        default:
            debug(DEBUG_WARNING, "unknow frame type");
            return NET_ERROR_NOT_SUPPORT;
    }

    pkt_buf_free(buf);

    return NET_OK;
}

net_status_t ether_out(struct _netif_t *netif, ipaddr_t *dest, pkt_buf_t *buf) {

    if (ipaddr_is_equal(&netif->ipaddr, dest)) {
        return ether_raw_out(netif, NET_PROTOCOL_IPv4, netif->hwaddr.addr, buf);
    }

    const uint8_t *hwaddr = arp_find(netif, dest);
    if (hwaddr) {
        return ether_raw_out(netif, NET_PROTOCOL_IPv4, hwaddr, buf);
    } else {
        return arp_resolve(netif, dest, buf);
    }
}

net_status_t ether_init(void) {
    static const link_layer_t link_layer = {
            .type = NETIF_TYPE_ETHER,
            .open = ether_open,
            .close = ether_close,
            .in = ether_in,
            .out = ether_out
    };


    debug(DEBUG_INFO, "init ether");

    net_status_t status = netif_register_layer(&link_layer);
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "register_error");
        return status;
    }

    debug(DEBUG_INFO, "init ether done.");
    return NET_OK;
}


const uint8_t *ether_broadcast_addr(void) {
    static const uint8_t broadcast[NETIF_HWADDR_SIZE] = {0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF};

    return broadcast;
}

net_status_t ether_raw_out(netif_t *netif, protocol_t protocol, const uint8_t *dest_hwaddr, pkt_buf_t *buf) {
    net_status_t status;
    int size = buf->total_size;
    if (size < ETHER_DATA_MIN) {
        debug(DEBUG_INFO, "resize from %d to %d", size, ETHER_DATA_MIN);
        status = pkt_buf_resize(buf, ETHER_DATA_MIN);
        if (status < NET_OK) {
            debug(DEBUG_ERROR, "resize error");
            return status;
        }

        pkt_buf_reset_acc(buf);
        pkt_buf_seek(buf, size);
        pkt_buf_fill(buf, 0, ETHER_DATA_MIN - size);

        size = ETHER_DATA_MIN;
    }

    status = pkt_buf_add_header(buf, sizeof(ether_hdr_t), CONTINUE);
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "add header error: %d", status);
        return NET_ERROR_SIZE;
    }

    ether_pkt_t *pkt = (ether_pkt_t *) pkt_buf_data(buf);
    plat_memcpy(pkt->hdr.dest, dest_hwaddr, NETIF_HWADDR_SIZE);
    plat_memcpy(pkt->hdr.src, netif->hwaddr.addr, NETIF_HWADDR_SIZE);
    pkt->hdr.protocol = x_htons(protocol);

    display_ether_pkt("ether out", pkt, size);

    if (plat_memcmp(netif->hwaddr.addr, dest_hwaddr, NETIF_HWADDR_SIZE) == 0) {
        return netif_put_in(netif, buf, -1);
    } else {

        status = netif_put_out(netif, buf, -1);
        if (status < NET_OK) {
            debug(DEBUG_WARNING, "put pkt out failed");
            return status;
        }

        return netif->ops->xmit(netif);
    }
}

uint8_t ipaddr_is_equal(const ipaddr_t *src_ipaddr, const ipaddr_t *dest_ipaddr) {
    return src_ipaddr->q_addr == dest_ipaddr->q_addr;
}
