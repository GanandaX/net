//
// Created by Administrator on 2025/6/25.
//
#include "icmpv4.h"
#include "ipv4.h"
#include "protocol.h"


#if DEBUG_DISP_ENABLED(DEBUG_ICMPV4)

static void display_icmp(char *title, icmpv4_pkt_t *pkt) {
    icmpv4_hdr_t *hdr = &pkt->hdr;

    plat_printf("------------%15s------------\n", title);
    plat_printf("%15s %d\n", "type:", hdr->type);
    plat_printf("%15s %d\n", "code:", hdr->code);
    plat_printf("%15s %d\n", "checksum:", hdr->checksum);
    plat_printf("------------%15s end------------\n", title);
}

#else
#define display_icmp(title, pkt);
#endif


net_status_t icmpv4_init(void) {
    debug(DEBUG_INFO, "icmpv4 init");


    debug(DEBUG_INFO, "icmpv4 init done.");
    return NET_OK;
}

static net_status_t icmpv4_out(ipaddr_t *dest, ipaddr_t *src, pkt_buf_t *buf) {
    icmpv4_pkt_t *pkt = (icmpv4_pkt_t *) pkt_buf_data(buf);
    pkt_buf_reset_acc(buf);
    pkt->hdr.checksum = pkt_buf_checksum16(buf, buf->total_size, 0, 1);

    display_icmp("icmpv4 reply", pkt);

    return ipv4_out(NET_PROTOCOL_ICMPv4, dest, src, buf);
}

static net_status_t icmpv4_echo_reply(ipaddr_t *dest, ipaddr_t *src, pkt_buf_t *buf) {
    icmpv4_pkt_t *pkt = (icmpv4_pkt_t *) pkt_buf_data(buf);

    pkt->hdr.type = ICMPV4_ECHO_REPLY;
    pkt->hdr.checksum = 0;
    return icmpv4_out(dest, src, buf);
}

net_status_t is_pkt_ok(icmpv4_pkt_t *pkt, uint32_t size, pkt_buf_t *buf) {
    if (size < sizeof(icmpv4_hdr_t)) {
        debug(DEBUG_INFO, "is_pkt_ok: size < sizeof(icmpv4_hdr_t)=>%d < %d", size, sizeof(icmpv4_hdr_t));
        return NET_ERROR_SIZE;
    }

    uint16_t checksum = pkt_buf_checksum16(buf, size, 0, 1);
    if (checksum) {
        debug(DEBUG_INFO, "is_pkt_ok: bad checksum");
        return NET_ERROR_CHECKSUM;
    }

    return NET_OK;
}

net_status_t icmpv4_in(ipaddr_t *src, ipaddr_t *dst, pkt_buf_t *buf) {
    debug(DEBUG_INFO, "icmpv4 in");

    ipv4_pkt_t *ip_pkt = (ipv4_pkt_t *) pkt_buf_data(buf);

    int iphdr_size = ipv4_hdr_size(ip_pkt);
    net_status_t status = pkt_buf_set_continue_space(buf, iphdr_size + sizeof(icmpv4_hdr_t));
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "icmpv4 in: pkt_buf_set_continue_space failed");
        return status;
    }

    ip_pkt = (ipv4_pkt_t *) pkt_buf_data(buf);

    status = pkt_buf_remove_header(buf, iphdr_size);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "icmpv4 in: pkt_buf_remove_header failed");
        return status;
    }

    pkt_buf_reset_acc(buf);
    icmpv4_pkt_t *icmp_pkt = (icmpv4_pkt_t *) pkt_buf_data(buf);

    if ((status = is_pkt_ok(icmp_pkt, buf->total_size, buf)) < 0) {
        debug(DEBUG_WARNING, "icmpv4 in: is_pkt_ok failed");
        return status;
    }

    switch (icmp_pkt->hdr.type) {
        case ICMPV4_ECHO_REQUEST:
            display_icmp("icmpv4 echo", icmp_pkt);
            return icmpv4_echo_reply(src, dst, buf);

        default:
            debug(DEBUG_WARNING, "pkt free");
            pkt_buf_free(buf);
            return NET_OK;
    }
}

net_status_t icmpv4_out_unreach(ipaddr_t *dest_ip,  ipaddr_t *src_ip, uint8_t code, pkt_buf_t *ip_buf) {
    uint32_t copy_size = ipv4_hdr_size((ipv4_pkt_t *) (pkt_buf_data(ip_buf))) + 576;
    if (copy_size > ip_buf->total_size) {
        copy_size = ip_buf->total_size;
    }

    pkt_buf_t *new_buf = pkt_buf_alloc(copy_size + sizeof(icmpv4_hdr_t) + 4);
    if (!new_buf) {
        debug(DEBUG_WARNING, "icmpv4 out: pkt_buf_alloc failed");
        return NET_ERROR_NO_SOURCE;
    }

    icmpv4_pkt_t *pkt = (icmpv4_pkt_t *) pkt_buf_data(new_buf);
    pkt->hdr.type = ICMPV4_UNREACHABLE;
    pkt->hdr.code = code;
    pkt->hdr.checksum = 0;
    pkt->reserve = 0;

    pkt_buf_reset_acc(ip_buf);
    pkt_buf_seek(new_buf, sizeof(icmpv4_hdr_t) + 4);
    net_status_t status = pkt_buf_copy(new_buf, ip_buf, copy_size);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "icmpv4 out: pkt_buf_copy failed");
        pkt_buf_free(new_buf);
        return status;
    }

    status = icmpv4_out(dest_ip, src_ip, new_buf);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "icmpv4 out: icmpv4_out_unreach failed");
        pkt_buf_free(new_buf);
        return status;
    }

    return NET_OK;
}
