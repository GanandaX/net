//
// Created by Administrator on 2025/6/20.
//
#include "ipv4.h"
#include "debug.h"
#include "protocol.h"
#include "tools.h"
#include "netif.h"

static uint16_t packet_id = 0;

#if DEBUG_DISP_ENABLED(DEBUG_IP)

static void display_ip_entry(ipv4_pkt_t *pkt) {
    ipv4_header_t *ip_hdr = &pkt->hdr;

    plat_printf("------------ip------------\n");
    plat_printf("%15s %d\n", "version:", ip_hdr->version);
    plat_printf("%15s %d\n", "header len:", ipv4_hdr_size(pkt));
    plat_printf("%15s %d\n", "total len:", ip_hdr->total_len);
    plat_printf("%15s %d\n", "id:", ip_hdr->id);
    plat_printf("%15s %d\n", "ttl:", ip_hdr->ttl);
    plat_printf("%15s %d\n", "protocol:", ip_hdr->protocol);
    plat_printf("%15s %x\n", "checksum:", ip_hdr->hdr_checksum);
    dbg_dump_ip_buf("src ip:", ip_hdr->src_ip);
    plat_printf("\n");
    dbg_dump_ip_buf("dest ip:", ip_hdr->dest_ip);
    plat_printf("\n");
    plat_printf("------------ip end------------\n");
}

#else
#define display_ip_entry(pkt);
#endif


net_status_t ipv4_init(void) {
    debug(DEBUG_INFO, "init ip");

    debug(DEBUG_INFO, "ipv4 header len: %d", sizeof(ipv4_header_t));

    debug(DEBUG_INFO, "ipv4 init done.");
    return NET_OK;
}

static net_status_t is_pkt_ok(ipv4_pkt_t *pkt, uint32_t total_size, netif_t *netif) {
    if (pkt->hdr.version != NET_VERSION_IPV4) {
        debug(DEBUG_WARNING, "invalid ip version");
        return NET_ERROR_NOT_SUPPORT;
    }

    uint8_t hdr_len = ipv4_hdr_size(pkt);
    if (hdr_len < sizeof(ipv4_header_t)) {
        debug(DEBUG_WARNING, "invalid hdr len");
        return NET_ERROR_SIZE;
    }

    uint16_t total_len = x_ntohs(pkt->hdr.total_len);
    if (total_size < sizeof(ipv4_header_t) || (total_size < total_len)) {
        debug(DEBUG_WARNING, "ipv4 size error");
        return NET_ERROR_SIZE;
    }

    if (pkt->hdr.hdr_checksum) {
        uint16_t c = checksum16(pkt, hdr_len, 0, 1);
        if (c) {
            debug(DEBUG_WARNING, "checksum error");
            return NET_ERROR_PACKAGE;
        }
    }
    return NET_OK;
}

static void iphdr_ntohs(ipv4_pkt_t *pkt) {
    pkt->hdr.total_len = x_ntohs(pkt->hdr.total_len);
    pkt->hdr.id = x_ntohs(pkt->hdr.id);
    pkt->hdr.frag_all = x_ntohs(pkt->hdr.frag_all);
}

static void iphdr_htons(ipv4_pkt_t *pkt) {
    pkt->hdr.total_len = x_htons(pkt->hdr.total_len);
    pkt->hdr.id = x_htons(pkt->hdr.id);
    pkt->hdr.frag_all = x_htons(pkt->hdr.frag_all);
}

static net_status_t ip_normal_in(netif_t *netif, pkt_buf_t *buf, ipaddr_t *src, ipaddr_t *dst) {
    ipv4_pkt_t *pkt = (ipv4_pkt_t *) pkt_buf_data(buf);

    display_ip_entry(pkt);

    switch (pkt->hdr.protocol) {
        case NET_PROTOCOL_ICMPv4:
            break;
        case NET_PROTOCOL_UDP:
            break;
        case NET_PROTOCOL_TCP:
            break;

        default:
            debug(DEBUG_WARNING, "invalid protocol");
            break;
    }

    return NET_ERROR_UNREACH;
}

net_status_t ipv4_in(netif_t *netif, pkt_buf_t *buf) {
    debug(DEBUG_INFO, "ip in");

    net_status_t status = pkt_buf_set_continue_space(buf, sizeof(ipv4_header_t));
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "adjust header failed, err = %d", status);
        return status;
    }

    ipv4_pkt_t *pkt = (ipv4_pkt_t *) pkt_buf_data(buf);
    status = is_pkt_ok(pkt, buf->total_size, netif);
    if (status != NET_OK) {
        debug(DEBUG_WARNING, "packet is broken.");
        return status;
    }

    iphdr_ntohs(pkt);
    status = pkt_buf_resize(buf, pkt->hdr.total_len);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "ip resize failed, err = %d", status);
        return status;
    }

    ipaddr_t dest_ip, src_ip;
    ipaddr_from_buf(&dest_ip, pkt->hdr.dest_ip);
    ipaddr_from_buf(&src_ip, pkt->hdr.src_ip);
    if (!ipaddr_is_match(&dest_ip, &netif->ipaddr, &netif->netmask)) {
        debug(DEBUG_ERROR, "ipaddr not match");
        return NET_ERROR_UNREACH;
    }

    status = ip_normal_in(netif, buf, &src_ip, &dest_ip);

    pkt_buf_free(buf);
    return NET_OK;
}


net_status_t ipv4_out(uint8_t protocol, ipaddr_t *dest, ipaddr_t *src, pkt_buf_t *buf) {
    debug(DEBUG_INFO, "send an ip pkt");

    net_status_t status = pkt_buf_add_header(buf, sizeof(ipv4_header_t), CONTINUE);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "add header failed, err = %d", status);
        return status;
    }

    ipv4_pkt_t *pkt = (ipv4_pkt_t *) pkt_buf_data(buf);
    pkt->hdr.shdr_all = 0;
    pkt->hdr.version = NET_VERSION_IPV4;
    ipv4_set_hdr_size(pkt, sizeof(ipv4_header_t));
    pkt->hdr.total_len = pkt_buf_total(buf);
    pkt->hdr.id = packet_id++;
    pkt->hdr.frag_all = 0;
    pkt->hdr.ttl = NET_IP_DEFAULT_TTL;
    pkt->hdr.protocol = protocol;
    pkt->hdr.hdr_checksum = 0;
    ipaddr_to_buf(src, pkt->hdr.src_ip);
    ipaddr_to_buf(dest, pkt->hdr.dest_ip);

    iphdr_htons(pkt);
    pkt_buf_reset_acc(buf);
    pkt->hdr.hdr_checksum = pkt_buf_checksum16(buf, ipv4_hdr_size(pkt), 0, 1);

    display_ip_entry(pkt);

    status = netif_out(netif_get_default(), dest, buf);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "ip out failed, err = %d", status);
        return status;
    }

    return NET_OK;
}
