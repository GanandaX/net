//
// Created by GrandBand on 2025/6/15.
//

#include "arp.h"
#include "m_block.h"
#include "tools.h"
#include "protocol.h"
#include "timer.h"
#include "ipv4.h"

#define to_scan_cnt(tmo) (tmo / ARP_TIMER_TMO)

static net_timer_t cache_timer;

static arp_entry_t cache_table[ARP_CACHE_SIZE];
static m_block_t cache_block;
static n_list_t cache_list;
static uint8_t empty_hwadd[] = {0, 0, 0, 0, 0, 0};

#if DEBUG_DISP_ENABLED(DEBUG_ARP)

static void display_arp_entry(arp_entry_t *entry) {
    plat_printf(" %d: ", (int) (entry - cache_table));
    dbg_dump_ip_buf(" ip:", entry->paddr);
    dbg_dump_hwaddr(" mac:", entry->hwaddr, NETIF_HWADDR_SIZE);

    plat_printf("%8s %d, %8s %d, %s, %8s %d\n", "tmo", entry->tmo, "retry", entry->retry,
                (entry->state == NET_ARP_RESOLVED) ? "stable" : "pending", "buf", n_list_count(&entry->buf_list));
}

static void display_arp_table(void) {
    plat_printf("-------- arp table -------\n");

    arp_entry_t *entry = cache_table;
    for (int i = 0; i < ARP_CACHE_SIZE; ++i, entry++) {
        if ((entry->state != NET_ARP_WAITING) && (entry->state != NET_ARP_RESOLVED)) {
            continue;
        }
        display_arp_entry(entry);
    }

    plat_printf("-------- arp end -------\n");

}

static void arp_pkt_display(arp_pkt_t *packet) {
    uint16_t opcode = x_ntohs(packet->opcode);

    plat_printf("-------- arp packet -------\n");
    plat_printf("%8s %d\n", "htype:", x_ntohs(packet->htype));
    plat_printf("%8s %04x\n", "ptype:", x_ntohs(packet->ptype));
    plat_printf("%8s %d\n", "hlen:", packet->hwlen);
    plat_printf("%8s %d\n", "plen:", packet->plen);
    plat_printf("%8s %d", "opscode:", opcode);
    switch (opcode) {
        case ARP_REPLY:
            plat_printf("%8s", "reply");
            break;
        case ARP_REQUEST:
            plat_printf("%8s", "request");
            break;
        default:
            plat_printf("%8s", "unknow");
            break;
    }

    dbg_dump_ip_buf("\nsender:", packet->sender_paddr);
    dbg_dump_hwaddr("mac:", packet->sender_hwaddr, NETIF_HWADDR_SIZE);
    dbg_dump_ip_buf("\ntarget:", packet->target_paddr);
    dbg_dump_hwaddr("mac:", packet->target_hwaddr, NETIF_HWADDR_SIZE);

    plat_printf("\n-------- arp end -------\n");
}

#else
#define display_arp_entry(entry);
#define display_arp_table();
#define arp_pkt_display(packet);
#endif

static net_status_t cache_init(void) {
    n_list_init(&cache_list);

    plat_memset(cache_table, 0, sizeof(cache_table));

    net_status_t status = m_block_init(&cache_block, cache_table, sizeof(arp_entry_t), ARP_CACHE_SIZE, N_LOCKER_NONE);
    if (status < NET_OK) {
        return status;
    }

    return NET_OK;
}

static void cache_clear_all(arp_entry_t *entry) {
    debug(DEBUG_INFO, "clear packet");

    n_list_node_t *node;
    while ((node = n_list_remove_first(&entry->buf_list))) {
        pkt_buf_t *buf = n_list_entity(node, node, pkt_buf_t);
        pkt_buf_free(buf);
    }
}

/**
 *
 * @param force 是否强制插入 0-否，1-是
 * @return
 */
static arp_entry_t *cache_alloc(uint8_t force) {
    arp_entry_t *entry = m_block_alloc(&cache_block, -1);
    if (!entry && force) {
        n_list_node_t *node = n_list_remove_last(&cache_list);
        if (!node) {
            debug(DEBUG_ERROR, "force alloc arp entry failed.");
            return (arp_entry_t *) 0;
        }
        entry = n_list_entity(node, node, arp_entry_t);
        cache_clear_all(entry);
    }

    if (entry) {
        plat_memset(entry, 0, sizeof(arp_entry_t));
        entry->state = NET_ARP_FREE;
        n_list_node_init(&entry->node);
        n_list_init(&entry->buf_list);
        return entry;
    }

    debug(DEBUG_ERROR, "alloc arp entry failed.");
    return (arp_entry_t *) 0;
}

static void cache_free(arp_entry_t *entry) {
    cache_clear_all(entry);
    n_list_remove(&cache_list, &entry->node);

    m_block_free(&cache_block, entry);
}

static arp_entry_t *cache_find(uint8_t *ip) {
    n_list_node_t *node;
    n_list_for_each(node, &cache_list) {
        arp_entry_t *entry = n_list_entity(node, node, arp_entry_t);
        if (plat_memcmp(ip, entry->paddr, IPV4_ADDR_SIZE) == 0) {
            n_list_remove(&cache_list, &entry->node);
            n_list_insert_first(&cache_list, &entry->node);
            return entry;
        }
    }

    return (arp_entry_t *) 0;
}

static net_status_t cache_send_all(arp_entry_t *entry) {
    dbg_dump_ip_buf("send all packet  ip:", entry->paddr);
    plat_printf("\n");

    n_list_node_t *node;
    while ((node = n_list_remove_first(&entry->buf_list))) {
        pkt_buf_t *buf = n_list_entity(node, node, pkt_buf_t);
        net_status_t status = ether_raw_out(entry->netif, NET_PROTOCOL_IPv4, entry->hwaddr, buf);
        if (status < NET_OK) {
            debug(DEBUG_WARNING, "ether raw out error: %d", status);
            pkt_buf_free(buf);
        }
    }

    return NET_OK;
}

static void cache_entry_set(arp_entry_t *entry, const uint8_t *hwaddr,
                            uint8_t *ip, netif_t *netif, uint8_t state) {
    plat_memcpy(entry->hwaddr, hwaddr, NETIF_HWADDR_SIZE);
    plat_memcpy(entry->paddr, ip, IPV4_ADDR_SIZE);
    entry->state = state;
    entry->netif = netif;

    if (state == NET_ARP_RESOLVED) {
        entry->tmo = to_scan_cnt(ARP_ENTRY_STABLE_TMO);
    } else {
        entry->tmo = to_scan_cnt(ARP_ENTRY_PENDING_TMO);
    }
    entry->retry = ARP_ENTRY_RETRY_CNT;
}

static net_status_t cache_insert(netif_t *netif, uint8_t *ip, uint8_t *hw_addr, uint8_t force) {
    if (*(uint32_t *) ip == 0) {
        return NET_ERROR_NOT_SUPPORT;
    }

    arp_entry_t *entry = cache_find(ip);

    if (entry) {
        dbg_dump_ip_buf("update arp entry, ip: ", ip);
        dbg_dump_hwaddr("sender mac: ", hw_addr, NETIF_HWADDR_SIZE);
        plat_printf("\n");

        cache_entry_set(entry, hw_addr, ip, netif, NET_ARP_RESOLVED);
        if (n_list_first(&cache_list) != &entry->node) {
            n_list_remove(&cache_list, &entry->node);
            n_list_insert_first(&cache_list, &entry->node);
        }
        net_status_t status = cache_send_all(entry);
        if (status < NET_OK) {
            debug(DEBUG_WARNING, "send packet failed.");
        }
        return status;
    } else {
        entry = cache_alloc(force);
        if (!entry) {
            dbg_dump_ip_buf("alloc failed. ip: ", ip);
            return NET_ERROR_NO_SOURCE;
        }
        cache_entry_set(entry, hw_addr, ip, netif, NET_ARP_RESOLVED);
        n_list_insert_first(&cache_list, &entry->node);
        dbg_dump_ip_buf("insert an entry entry, sender ip: ", ip);
        plat_printf("\n");
    }

    display_arp_table();
    return NET_OK;
}

static void arp_cache_tmo(net_timer_t *timer, void *arg) {
    uint8_t change_cnt = 0;
    n_list_node_t *node = (void *) 0;
    n_list_node_t *next_node = (void *) 0;

    for (node = cache_list.first; node; node = next_node) {
        next_node = n_list_node_next(node);

        arp_entry_t *entry = n_list_entity(node, node, arp_entry_t);
        if (--entry->tmo > 0) {
            continue;
        }

        change_cnt++;
        switch (entry->state) {
            case NET_ARP_RESOLVED:
                debug(DEBUG_INFO, "state to pending");
                display_arp_entry(entry);

                ipaddr_t ipaddr;
                ipaddr_from_buf(&ipaddr, entry->paddr);
                entry->state = NET_ARP_WAITING;
                entry->tmo = to_scan_cnt(ARP_ENTRY_PENDING_TMO);
                entry->retry = ARP_ENTRY_RETRY_CNT;
                arp_make_request(entry->netif, &ipaddr);
                break;
            case NET_ARP_WAITING:

                if (--entry->retry == 0) {
                    debug(DEBUG_WARNING, "pending tmo, free it!");
                    display_arp_entry(entry);
                    cache_free(entry);
                } else {
                    debug(DEBUG_INFO, "pending tmo, send request");
                    display_arp_entry(entry);

                    ipaddr_t ipaddr;
                    ipaddr_from_buf(&ipaddr, entry->paddr);
                    entry->tmo = to_scan_cnt(ARP_ENTRY_PENDING_TMO);
                    arp_make_request(entry->netif, &ipaddr);
                }
                break;
            default:
                debug(DEBUG_WARNING, "unknown arp state");
                display_arp_entry(entry);
                break;
        }
    }

    if (change_cnt) {
        debug(DEBUG_INFO, "%d arp entry changed", change_cnt);
        display_arp_table();
    }
}

net_status_t arp_init() {
    net_status_t status = cache_init();
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "arp cache init failed.");
    }

    status = net_timer_add(&cache_timer, "arp timer", arp_cache_tmo, (void *) 0, ARP_TIMER_TMO * 1000, 1);
    if (status < NET_OK) {
        debug(DEBUG_WARNING, "create timer failed: %d", status);
        return status;
    }

    return NET_OK;
}

net_status_t arp_make_request(netif_t *netif, const ipaddr_t *dest_ip) {
    /*    uint8_t *ip = (uint8_t *) dest_ip->a_addr;
        ip[0] = 0x01;
        cache_insert(netif, ip, netif->hwaddr.addr, 1);
        ip[0] = 0x02;
        cache_insert(netif, ip, netif->hwaddr.addr, 1);
        ip[0] = 0x03;
        cache_insert(netif, ip, netif->hwaddr.addr, 1);
        cache_insert(netif, ip, netif->hwaddr.addr, 1);*/

    pkt_buf_t *buf = pkt_buf_alloc(sizeof(arp_pkt_t));
    if (buf == (pkt_buf_t *) 0) {
        debug(DEBUG_ERROR, "alloc arp buf error");
        return NET_ERROR_NO_SOURCE;
    }

    pkt_buf_set_continue_space(buf, sizeof(arp_pkt_t));
    arp_pkt_t *arp_packet = (arp_pkt_t *) pkt_buf_data(buf);

    arp_packet->htype = x_htons(ARP_HW_ETHER);
    arp_packet->ptype = x_htons(NET_PROTOCOL_IPv4);
    arp_packet->hwlen = NETIF_HWADDR_SIZE;
    arp_packet->plen = IPV4_ADDR_SIZE;
    arp_packet->opcode = x_htons(ARP_REQUEST);
    plat_memcpy(arp_packet->sender_hwaddr, netif->hwaddr.addr, NETIF_HWADDR_SIZE);
    ipaddr_to_buf(&netif->ipaddr, arp_packet->sender_paddr);
    plat_memset(arp_packet->target_hwaddr, 0, NETIF_HWADDR_SIZE);
    ipaddr_to_buf(dest_ip, arp_packet->target_paddr);

    net_status_t status = ether_raw_out(netif, NET_PROTOCOL_ARP, ether_broadcast_addr(), buf);
    if (status) {
        debug(DEBUG_WARNING, "ether raw out error: %d", status);
        pkt_buf_free(buf);
    }

    arp_pkt_display(arp_packet);

    return status;
}

net_status_t arp_make_reply(netif_t *netif, pkt_buf_t *buf) {
    arp_pkt_t *arp_packet = (arp_pkt_t *) pkt_buf_data(buf);

    arp_packet->opcode = x_htons(ARP_REPLY);

    plat_memcpy(arp_packet->target_hwaddr, arp_packet->sender_hwaddr, NETIF_HWADDR_SIZE);
    plat_memcpy(arp_packet->target_paddr, arp_packet->sender_paddr, IPV4_ADDR_SIZE);
    plat_memcpy(arp_packet->sender_hwaddr, netif->hwaddr.addr, NETIF_HWADDR_SIZE);
    ipaddr_to_buf(&netif->ipaddr, arp_packet->sender_paddr);

    arp_pkt_display(arp_packet);

    return ether_raw_out(netif, NET_PROTOCOL_ARP, arp_packet->target_hwaddr, buf);
}

static net_status_t is_pkt_ok(arp_pkt_t *arp_packet, uint16_t size, netif_t *netif) {
    if (size < sizeof(arp_pkt_t)) {
        debug(DEBUG_WARNING, "packet size error");
        return NET_ERROR_SIZE;
    }

    if ((x_ntohs(arp_packet->htype) != ARP_HW_ETHER) ||
        (arp_packet->hwlen != NETIF_HWADDR_SIZE) ||
        (x_ntohs(arp_packet->ptype) != NET_PROTOCOL_IPv4) ||
        (arp_packet->plen != IPV4_ADDR_SIZE)) {
        debug(DEBUG_WARNING, "packet incorrect");
        return NET_ERROR_NOT_SUPPORT;
    }

    uint32_t opcode = x_ntohs(arp_packet->opcode);
    if ((opcode != ARP_REQUEST && opcode != ARP_REPLY)) {
        debug(DEBUG_WARNING, "unknow operation code");
        return NET_ERROR_NOT_SUPPORT;
    }

    return NET_OK;
}

net_status_t arp_make_gratuitous(netif_t *netif) {
    debug(DEBUG_INFO, "send an gratuitous arp");
    return arp_make_request(netif, &netif->ipaddr);
}

net_status_t arp_in(netif_t *netif, pkt_buf_t *buf) {
    debug(DEBUG_INFO, "arp in");

    net_status_t status = pkt_buf_set_continue_space(buf, sizeof(arp_pkt_t));
    if (status < NET_OK) {
        return status;
    }

    arp_pkt_t *arp_packet = (arp_pkt_t *) pkt_buf_data(buf);
    status = is_pkt_ok(arp_packet, buf->total_size, netif);
    if (status != NET_OK) {
        return status;
    }

    arp_pkt_display(arp_packet);

    ipaddr_t target_ip;
    ipaddr_from_buf(&target_ip, arp_packet->target_paddr);
    if (ipaddr_is_equal(&netif->ipaddr, &target_ip)) {
        if (!hw_addr_is_equal(arp_packet->target_hwaddr, netif->hwaddr.addr)) {
            debug(DEBUG_INFO, "received an arp for me buf mac error, send a gratuitous");
            return arp_make_gratuitous(netif);
        }

        debug(DEBUG_INFO, "received an arp for me, force update");

        cache_insert(netif, arp_packet->sender_paddr, arp_packet->sender_hwaddr, 1);
        if (x_ntohs(arp_packet->opcode) == ARP_REQUEST) {
            debug(DEBUG_INFO, "arp request, send reply");
            return arp_make_reply(netif, buf);
        }
    } else {
        debug(DEBUG_INFO, "received an arp not for me, try to update.");
        cache_insert(netif, arp_packet->sender_paddr, arp_packet->sender_hwaddr, 0);
    }

    debug(DEBUG_WARNING, "pkt free");
    pkt_buf_free(buf);
    return NET_OK;
}

net_status_t arp_resolve(netif_t *netif, ipaddr_t *ipaddr, pkt_buf_t *buf) {
    arp_entry_t *entry = cache_find(ipaddr->a_addr);

    if (entry) {
        debug(DEBUG_INFO, "found an arp entry");

        if (entry->state == NET_ARP_RESOLVED) {
            return ether_raw_out(netif, NET_PROTOCOL_IPv4, entry->hwaddr, buf);
        }

        if (n_list_count(&entry->buf_list) < ARP_MAX_PKT_WAIT) {
            debug(DEBUG_INFO, "insert buf to arp entry");

            n_list_insert_last(&entry->buf_list, &buf->node);
            return NET_OK;
        } else {
            debug(DEBUG_WARNING, "too many waiting.");
            return NET_ERROR_FULL;
        }
    } else {
        debug(DEBUG_INFO, "make arp request");

        entry = cache_alloc(1);
        cache_entry_set(entry, empty_hwadd, ipaddr->a_addr, netif, NET_ARP_WAITING);
        n_list_insert_first(&cache_list, &entry->node);

        n_list_insert_last(&entry->buf_list, &buf->node);

        display_arp_table();

        return arp_make_request(netif, ipaddr);
    }
}

void arp_clear(netif_t *netif) {
    n_list_node_t *node, *next;
    for (node = n_list_first(&cache_list); node; node = next) {
        next = n_list_node_next(node);

        arp_entry_t *entry = n_list_entity(node, node, arp_entry_t);
        if (entry->netif == netif) {
            cache_clear_all(entry);
            n_list_remove(&cache_list, node);
        }
    }
}

const uint8_t *arp_find(netif_t *netif, ipaddr_t *ipaddr) {
    if (ipaddr_is_local_broadcast(ipaddr) || ipaddr_is_direct_broadcast(ipaddr, &netif->netmask)) {
        return ether_broadcast_addr();
    }

    arp_entry_t *entry = cache_find(ipaddr->a_addr);
    if (entry && (entry->state == NET_ARP_RESOLVED)) {
        return entry->hwaddr;
    }

    return (const uint8_t *) 0;
}

void arp_update_from_ipbuf(netif_t *netif, pkt_buf_t *buf) {
    net_status_t status = pkt_buf_set_continue_space(buf, sizeof(ipv4_header_t) + sizeof(ether_hdr_t));
    if (status != NET_OK) {
        debug(DEBUG_WARNING, "adjust header failed.");
        return;
    }

    pkt_buf_reset_acc(buf);
    ether_hdr_t *ether_hdr = (ether_hdr_t *) pkt_buf_data(buf);
    ipv4_header_t *ipv4_hdr = (ipv4_header_t *) (sizeof(ether_hdr_t) + (uint8_t *) ether_hdr);
    if (ipv4_hdr->version != NET_VERSION_IPV4) {
        debug(DEBUG_WARNING, "wrong ipv4 version");
        return;
    }

    cache_insert(netif, ipv4_hdr->src_ip, ether_hdr->src, 0);
}
