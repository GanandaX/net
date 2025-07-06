//
// Created by Administrator on 2025/6/20.
//
#include "ipv4.h"
#include "debug.h"
#include "ether.h"
#include "icmpv4.h"
#include "m_block.h"
#include "protocol.h"
#include "tools.h"
#include "netif.h"
#include "raw.h"

static uint16_t packet_id = 0;
static ip_frag_t frag_array[IP_FRAGS_MAX_CNT];
static m_block_t frag_m_block;
static n_list_t frag_list;

#if DEBUG_DISP_ENABLED(DEBUG_IP)
static void display_ip_entry(ipv4_pkt_t *pkt) {
    ipv4_header_t *ip_hdr = &pkt->hdr;

    plat_printf("------------ip------------\n");
    plat_printf("%15s %d\n", "version:", ip_hdr->version);
    plat_printf("%15s %d\n", "header len:", ipv4_hdr_size(pkt));
    plat_printf("%15s %d\n", "total len:", ip_hdr->total_len);
    plat_printf("%15s %d\n", "id:", ip_hdr->id);
    plat_printf("%15s %d\n", "ttl:", ip_hdr->ttl);
    plat_printf("%15s %d\n", "frag offset:", ip_hdr->frag_offset);
    plat_printf("%15s %d\n", "frag more:", ip_hdr->more);
    plat_printf("%15s %d\n", "protocol:", ip_hdr->protocol);
    plat_printf("%15s %x\n", "checksum:", ip_hdr->hdr_checksum);
    dbg_dump_ip_buf("src ip:", ip_hdr->src_ip);
    plat_printf("\n");
    dbg_dump_ip_buf("dest ip:", ip_hdr->dest_ip);
    plat_printf("\n");
    plat_printf("------------ip end------------\n");
}

static uint32_t get_data_size(ipv4_pkt_t *pkt) {
    return pkt->hdr.total_len - ipv4_hdr_size(pkt);
}

static uint16_t get_frag_start(ipv4_pkt_t *pkt) {
    return pkt->hdr.frag_offset * 8;
}

static uint16_t get_frag_end(ipv4_pkt_t *pkt) {
    return get_frag_start(pkt) + get_data_size(pkt);
}

static void display_ip_frags(void) {
    plat_printf("------------ frag ------------\n");

    uint8_t f_index = 0;
    n_list_node_t *frag_node;
    n_list_for_each(frag_node, &frag_list) {
        ip_frag_t *frag = n_list_entity(frag_node, node, ip_frag_t);

        plat_printf("[%d]:    ", f_index++);
        dbg_dump_ip("ip: ", &frag->ip);
        plat_printf("%15s %X(%d)", "id:", frag->id, frag->id);
        plat_printf("%15s %d", "tom:", frag->tmo);
        plat_printf("%15s %d\n", "buf len:", frag->buf_list.count);
        plat_printf("%15s\n", "bufs:");

        n_list_node_t *p_node;
        uint8_t p_index = 0;
        n_list_for_each(p_node, &frag->buf_list) {
            pkt_buf_t *buf = n_list_entity(p_node, node, pkt_buf_t);

            ipv4_pkt_t *pkt = (ipv4_pkt_t *) pkt_buf_data(buf);
            plat_printf("%20s %d:[%d-%d]", "", p_index++, get_frag_start(pkt), get_frag_end(pkt) - 1);
        }

        plat_printf("\n");
        plat_printf("\n");
    }


    plat_printf("------------ frag end ------------\n");
}

#else
#define display_ip_entry(pkt);
#define display_ip_frags(void);
#define get_data_size(pkt);
#define get_frag_start(pkt);
#define get_frag_end(pkt);
#endif

static net_status_t frag_init() {
    n_list_init(&frag_list);
    m_block_init(&frag_m_block, frag_array, sizeof(ip_frag_t), IP_FRAGS_MAX_CNT, N_LOCKER_NONE);

    return NET_OK;
}

static void frag_free_buf_list(ip_frag_t *frag) {
    n_list_node_t *node;

    while ((node = n_list_remove_first(&frag->buf_list))) {
        pkt_buf_t *buf = n_list_entity(node, node, pkt_buf_t);
        pkt_buf_free(buf);
    }
}

static void frag_free(ip_frag_t *frag) {
    frag_free_buf_list(frag);

    n_list_remove(&frag_list, &frag->node);
    m_block_free(&frag_m_block, frag);
}

static ip_frag_t *frag_alloc() {
    ip_frag_t *frag = m_block_alloc(&frag_m_block, -1);
    if (!frag) {
        n_list_node_t *node = n_list_remove_last(&frag_list);
        frag = n_list_entity(node, node, ip_frag_t);
        if (frag) {
            frag_free_buf_list(frag);
        }
    }
    return frag;
}


net_status_t ipv4_init(void) {
    debug(DEBUG_INFO, "init ip");

    net_status_t status = frag_init();
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "frag_init failed");
        return status;
    }

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

static uint8_t frag_is_all_arrived(ip_frag_t *frag) {
    uint32_t offset = 0;

    ipv4_pkt_t *pkt = (ipv4_pkt_t *) 0;
    n_list_node_t *node;
    n_list_for_each(node, &frag->buf_list) {
        pkt_buf_t *buf = n_list_entity(node, node, pkt_buf_t);
        pkt = (ipv4_pkt_t *) pkt_buf_data(buf);

        int curr_offset = get_frag_start(pkt);
        if (curr_offset != offset) {
            return 0;
        }

        offset += get_data_size(pkt);
    }

    return pkt ? !pkt->hdr.more : 0;
}

static pkt_buf_t *frag_join(ip_frag_t *frag) {
    pkt_buf_t *target = (pkt_buf_t *) 0;

    n_list_node_t *node;
    while ((node = n_list_remove_first(&frag->buf_list))) {
        pkt_buf_t *curr = n_list_entity(node, node, pkt_buf_t);

        if (!target) {
            target = curr;
            continue;
        }

        ipv4_pkt_t *pkt = (ipv4_pkt_t *) pkt_buf_data(curr);
        net_status_t status = pkt_buf_remove_header(curr, ipv4_hdr_size(pkt));
        if (status < NET_OK) {
            debug(DEBUG_WARNING, "frag_join remove header failed");
            pkt_buf_free(curr);
            goto free_and_return;
        }

        status = pkt_buf_join(target, curr);
        if (status < NET_OK) {
            debug(DEBUG_WARNING, "frag_join join failed");
            pkt_buf_free(curr);
            goto free_and_return;
        }
    }
    frag_free(frag);
    return target;

free_and_return:

    if (target) {
        pkt_buf_free(target);
    }
    frag_free(frag);
    return (pkt_buf_t *) 0;
}

static ip_frag_t *frag_find(ipaddr_t *ip, uint16_t id) {
    n_list_node_t *curr;

    n_list_for_each(curr, &frag_list) {
        ip_frag_t *frag = n_list_entity(curr, node, ip_frag_t);

        if (ipaddr_is_equal(ip, &frag->ip) && (id == frag->id)) {
            n_list_remove(&frag_list, &frag->node);
            n_list_insert_first(&frag_list, curr);
            return frag;
        }
    }

    return (ip_frag_t *) 0;
}

static net_status_t frag_add(ip_frag_t *frag, ipaddr_t *ip, uint16_t id) {
    ipaddr_copy(&frag->ip, ip);
    frag->tmo = 0;
    frag->id = id;

    n_list_node_init(&frag->node);
    n_list_init(&frag->buf_list);

    n_list_insert_first(&frag_list, &frag->node);
}

static net_status_t frag_insert(ip_frag_t *frag, pkt_buf_t *buf, ipv4_pkt_t *pkt) {
    if (n_list_count(&frag->buf_list) >= IP_FRAG_MAX_BUF_CNT) {
        debug(DEBUG_WARNING, "too many bufs on frag");
        return NET_ERROR_FULL;
    }

    n_list_node_t *node;
    n_list_for_each(node, &frag->buf_list) {
        pkt_buf_t *curr_buf = n_list_entity(node, node, pkt_buf_t);
        ipv4_pkt_t *curr_pkt = (ipv4_pkt_t *) pkt_buf_data(curr_buf);

        uint16_t curr_start = get_frag_start(curr_pkt);
        if (get_frag_start(pkt) == curr_start) {
            return NET_ERROR_EXIST;
        } else if (get_frag_end(pkt) <= curr_start) {
            n_list_node_t *pre = n_list_node_pre(node);
            if (pre) {
                n_list_insert_after(&frag->buf_list, pre, &buf->node);
            } else {
                n_list_insert_first(&frag->buf_list, &buf->node);
            }

            return NET_OK;
        }
    }

    n_list_insert_last(&frag->buf_list, &buf->node);
    return NET_OK;
}

static net_status_t ip_normal_in(netif_t *netif, pkt_buf_t *buf, ipaddr_t *src, ipaddr_t *dst) {
    ipv4_pkt_t *pkt = (ipv4_pkt_t *) pkt_buf_data(buf);

    display_ip_entry(pkt);

    switch (pkt->hdr.protocol) {
        case NET_PROTOCOL_ICMPv4:
            net_status_t status = icmpv4_in(src, &netif->ipaddr, buf);
            if (status < NET_OK) {
                debug(DEBUG_WARNING, "icmpv4_in error");
            }
            return status;
        case NET_PROTOCOL_UDP:
            iphdr_htons(pkt);
            status = icmpv4_out_unreach(src, &netif->ipaddr, ICMPV4_UNREACH_PORT, buf);
            if (status < NET_OK) {
                debug(DEBUG_WARNING, "icmpv4_out error");
                return status;
            }
            pkt_buf_free(buf);
            return NET_OK;
        case NET_PROTOCOL_TCP:
            break;

        default:
            status =  raw_in(buf);
            if (status < 0) {
                debug(DEBUG_WARNING, "raw is error");
                return status;
            }
            break;
    }

    return NET_ERROR_UNREACH;
}

static net_status_t ip_frag_in(netif_t *netif, pkt_buf_t *buf, ipaddr_t *src_ip, ipaddr_t *dest_ip) {
    ipv4_pkt_t *curr = (ipv4_pkt_t *) pkt_buf_data(buf);

    ip_frag_t *frag = frag_find(src_ip, curr->hdr.id);
    if (!frag) {
        frag = frag_alloc();
        frag_add(frag, src_ip, curr->hdr.id);
    }

    net_status_t status = frag_insert(frag, buf, curr);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "frag_in failed");
        return status;
    }

    if (frag_is_all_arrived(frag)) {
        pkt_buf_t *full_buf = frag_join(frag);
        if (!full_buf) {
            debug(DEBUG_WARNING, "frag_join failed");
            display_ip_frags();
            return NET_OK;
        }

        status = ip_normal_in(netif, full_buf, src_ip, dest_ip);
        if (status < NET_OK) {
            debug(DEBUG_WARNING, "ip_normal_in failed");
            pkt_buf_free(full_buf);
            return NET_OK;
        }
    }

    display_ip_frags();
    return NET_OK;
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

    if (pkt->hdr.frag_offset || pkt->hdr.more) {
        status = ip_frag_in(netif, buf, &src_ip, &dest_ip);
    } else {
        status = ip_normal_in(netif, buf, &src_ip, &dest_ip);
    }
    return status;
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
