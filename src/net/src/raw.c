//
// Created by Administrator on 2025/6/30.
//
#include "raw.h"

#include "ether.h"
#include "ipv4.h"
#include "m_block.h"
#include "socket.h"

static raw_t raw_table[RAW_MAX_CNT];
static m_block_t raw_m_block;
static n_list_t raw_list;


#if DEBUG_DISP_ENABLED(DEBUG_RAW)

static void display_raw_list(void) {
    plat_printf("--------raw list---------\n");

    n_list_node_t *node;
    int idx = 0;
    n_list_for_each(node, &raw_list) {
        raw_t *raw = (raw_t *) n_list_entity(node, node, sock_t);

        plat_printf("[%d]: ", idx++);
        dbg_dump_ip("local:", &raw->base.local_ip);
        dbg_dump_ip("remote:", &raw->base.local_ip);
    }

    plat_printf("\n--------raw list end---------\n");

}
#else
#define display_raw_list(void);
#endif


net_status_t raw_init(void) {
    debug(DEBUG_INFO, "raw init");

    n_list_init(&raw_list);
    m_block_init(&raw_m_block, raw_table, sizeof(raw_t), RAW_MAX_CNT, N_LOCKER_NONE);

    debug(DEBUG_INFO, "raw init done.");
    return NET_OK;
}

static net_status_t raw_sendto(struct _sock_t *s, const void *buf, size_t len, int flags,
                               const struct x_sockaddr *dest, x_socklen_t dest_len, ssize_t *result_len) {
    pkt_buf_t *pkt_buf = pkt_buf_alloc((int) len);
    if (!pkt_buf) {
        debug(DEBUG_WARNING, "no buffer");
        return NET_ERROR_MEM;
    }

    net_status_t status = pkt_buf_write(pkt_buf, (uint8_t *) buf, (int) len);
    if (status < 0) {
        debug(DEBUG_ERROR, "copy data error");
        goto error_process;
    }

    ipaddr_t dest_ip;
    struct x_sockaddr_in *addr = (struct x_sockaddr_in *) dest;
    ipaddr_from_buf(&dest_ip, addr->sin_addr.addr_array);

    if (!ipaddr_is_any(&s->remote_ip) && !ipaddr_is_equal(&dest_ip, &s->remote_ip)) {
        debug(DEBUG_WARNING, "dest address is incorrect");
        return NET_ERROR_PARAM;
    }

    status = ipv4_out(s->protocol, &dest_ip, &netif_get_default()->ipaddr, pkt_buf);
    if (status < 0) {
        debug(DEBUG_ERROR, "ipv4_out() error");
        goto error_process;
    }

    *result_len = (ssize_t) len;

    return NET_OK;
error_process:
    pkt_buf_free(pkt_buf);
    return status;
}

static net_status_t raw_recvfrom(struct _sock_t *s, void *buf, size_t len, int flags,
                                 const struct x_sockaddr *src, x_socklen_t *src_len, ssize_t *result_len) {
    raw_t *raw = (raw_t *) s;
    n_list_node_t *first = n_list_remove_first(&raw->recv_list);
    if (!first) {
        *result_len = 0;
        return NET_ERROR_NEED_WAIT;
    }

    pkt_buf_t *pkt_buf = n_list_entity(first, node, pkt_buf_t);
    ipv4_header_t *iphdr = (ipv4_header_t *) pkt_buf_data(pkt_buf);
    struct x_sockaddr_in *addr = (struct x_sockaddr_in *) src;
    plat_memset(addr, 0, sizeof(struct x_sockaddr_in));
    addr->sin_family = AF_INET;
    plat_memcpy(&addr->sin_addr, iphdr->src_ip, IPV4_ADDR_SIZE);

    uint32_t size = pkt_buf->total_size > (uint32_t) len ? (uint32_t) len : pkt_buf->total_size;
    pkt_buf_reset_acc(pkt_buf);
    net_status_t status = pkt_buf_read(pkt_buf, buf, size);
    pkt_buf_free(pkt_buf);
    if (status < 0) {
        debug(DEBUG_WARNING, "pktbuf raw read error");
        return status;
    }

    *result_len = size;
    return NET_OK;
}


net_status_t raw_close(sock_t *sock) {
    raw_t *raw = (raw_t *) sock;

    n_list_remove(&raw_list, &sock->node);

    n_list_node_t *node;
    while (node = n_list_remove_first(&raw->recv_list)) {
        pkt_buf_t *pkt_buf = n_list_entity(node, node, pkt_buf_t);
        pkt_buf_free(pkt_buf);
    }

    sock_uninit(sock);
    m_block_free(&raw_m_block, raw);

    display_raw_list();
    return NET_OK;
}

sock_t *raw_create(int family, int protocol) {
    static const sock_ops_t raw_ops = {
        .sendto = raw_sendto,
        .recvfrom = raw_recvfrom,
        .setopt = sock_setopt,
        .close = raw_close
    };

    raw_t *raw = m_block_alloc(&raw_m_block, -1);
    if (!raw) {
        debug(DEBUG_WARNING, "no raw sock");
        return (sock_t *) 0;
    }

    net_status_t status = sock_init(&raw->base, family, protocol, &raw_ops);

    if (status < 0) {
        debug(DEBUG_ERROR, "raw init failed.");
        m_block_free(&raw_m_block, raw);
        return (sock_t *) 0;
    }

    n_list_init(&raw->recv_list);
    raw->base.rcv_wait = &raw->recv_wait;
    if (sock_wait_init(raw->base.rcv_wait) < 0) {
        debug(DEBUG_ERROR, "raw init failed.");
        goto create_failed;
    }

    n_list_insert_last(&raw_list, &raw->base.node);

    display_raw_list();

    return (sock_t *) raw;

create_failed:
    sock_uninit(&raw->base);
    return (sock_t *) 0;
}


static raw_t *raw_find(ipaddr_t *src, ipaddr_t *dest, uint8_t protocol) {
    n_list_node_t *node;

    n_list_for_each(node, &raw_list) {
        raw_t *raw = (raw_t *) n_list_entity(node, node, sock_t);

        if (raw->base.protocol && raw->base.protocol != protocol) {
            continue;
        }

        if (!ipaddr_is_any(&raw->base.remote_ip) && !ipaddr_is_equal(&raw->base.remote_ip, src)) {
            continue;
        }

        if (!ipaddr_is_any(&raw->base.local_ip) && !ipaddr_is_equal(&raw->base.local_ip, dest)) {
            continue;
        }

        return raw;
    }

    return (raw_t *) 0;
}

net_status_t raw_in(pkt_buf_t *pkt_buf) {
    ipv4_header_t *iphdr = (ipv4_header_t *) pkt_buf_data(pkt_buf);

    ipaddr_t src, dest;
    ipaddr_from_buf(&dest, iphdr->dest_ip);
    ipaddr_from_buf(&src, iphdr->src_ip);

    raw_t *raw = raw_find(&src, &dest, iphdr->protocol);
    if (!raw) {
        debug(DEBUG_WARNING, "raw null for this packet");
        return NET_ERROR_UNREACH;
    }

    n_list_insert_last(&raw->recv_list, &pkt_buf->node);
    sock_wakeup(&raw->base, SOCK_WAIT_READ, NET_OK);
    return NET_OK;
}
