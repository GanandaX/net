//
// Created by Administrator on 2025/6/6.
//


#include "netif.h"
#include "m_block.h"
#include "pkt_buf.h"
#include "ex_msg.h"

static netif_t netif_buffer[NETIF_DEV_CNT];
static m_block_t netif_block;
static n_list_t netif_list;
static netif_t *netif_default;

#if DEBUG_DISP_ENABLED(DEBUG_NETIF)

static void display_netif_list() {
    plat_printf("netif list: \n");

    n_list_node_t *node;
    n_list_for_each(node, &netif_list) {
        netif_t *netif = n_list_entity(node, node, netif_t);

        plat_printf("%s:", netif->name);
        switch (netif->state) {
            case NETIF_CLOSED:
                plat_printf("\tclosed\t");
                break;
            case NETIF_OPENED:
                plat_printf("\topened\t");
                break;
            case NETIF_ACTIVE:
                plat_printf("\tactive\t");
                break;
            default:
                break;
        }
        switch (netif->type) {
            case NETIF_TYPE_NONE:
                plat_printf("\tnone\t");
                break;
            case NETIF_TYPE_ETHER:
                plat_printf("\tethernet\t");
                break;
            case NETIF_TYPE_LOOP:
                plat_printf("\tloop\t");
                break;
            default:
                break;
        }

        plat_printf("\tmut=%d\t", netif->mtu);

        dbg_dump_hwaddr("\thwaddr: ", netif->hwaddr.addr, netif->hwaddr.len);
        dbg_dump_ip("\tip: ", &netif->ipaddr);
        dbg_dump_ip("\tmask: ", &netif->netmask);
        dbg_dump_ip("\tgateway: ", &netif->gateway);
        plat_printf("\n");
    }

}

#else
#define display_netif_list(void)
#endif


net_status_t netif_init(void) {

    debug(DEBUG_NETIF, "netif init");

    n_list_init(&netif_list);

    m_block_init(&netif_block, netif_buffer, sizeof(netif_t), NETIF_DEV_CNT, N_LOCKER_NONE);

    netif_default = (netif_t *) 0;

    debug(DEBUG_NETIF, "init done");
    return NET_OK;
}

netif_t *netif_open(const char *dev_name, struct _netif_ops_t *ops, void *ops_data) {

    netif_t *netif = (netif_t *) m_block_alloc(&netif_block, -1);

    if (!netif) {
        debug(DEBUG_ERROR, "no netif");
        return (netif_t *) 0;
    }

    plat_strncpy(netif->name, dev_name, NETIF_NAME_SIZE);
    netif->name[NETIF_NAME_SIZE - 1] = 0;

    plat_memset(&netif->hwaddr, 0, sizeof(netif_hwaddr_t));

    ipaddr_set_any(&netif->ipaddr);
    ipaddr_set_any(&netif->netmask);
    ipaddr_set_any(&netif->gateway);

    netif->type = NETIF_TYPE_NONE;
    netif->mtu = 0;

    n_list_node_init(&netif->node);

    net_status_t status = fix_queue_init(&netif->in_q, netif->in_q_buf, NETIF_INQ_SIZE, N_LOCKER_THREAD);
    if (status < 0) {
        debug(DEBUG_NETIF, "netif in_q init failed.");
        return (netif_t *) 0;
    }

    status = fix_queue_init(&netif->out_q, netif->out_q_buf, NETIF_OUTQ_SIZE, N_LOCKER_THREAD);
    if (status < 0) {
        debug(DEBUG_NETIF, "netif out_q init failed.");
        fix_queue_destroy(&netif->in_q);
        m_block_free(&netif_block, netif);
        return (netif_t *) 0;
    }

    status = ops->open(netif, ops_data);
    if (status < NET_OK) {
        debug(DEBUG_ERROR, "netif ops open error");
        goto free_return;
    }

    netif->ops = ops;
    netif->ops_data = ops_data;
    netif->state = NETIF_OPENED;

    if (netif->type == NETIF_TYPE_NONE) {
        debug(DEBUG_ERROR, "netif type unknow");
        goto free_return;
    }

    n_list_insert_last(&netif_list, &netif->node);
    display_netif_list();

    return netif;

    free_return:
    if (netif->state == NETIF_OPENED) {
        netif->ops->close(netif);
    }

    fix_queue_destroy(&netif->in_q);
    fix_queue_destroy(&netif->out_q);
    m_block_free(&netif_block, netif);
    return (netif_t *) 0;
}


net_status_t netif_set_addr(netif_t *netif, ipaddr_t *ip, ipaddr_t *mask, ipaddr_t *gateway) {
    plat_memcpy(&netif->ipaddr, ip ? ip : ipaddr_get_empty(), sizeof(ipaddr_t));
    plat_memcpy(&netif->netmask, mask ? mask : ipaddr_get_empty(), sizeof(ipaddr_t));
    plat_memcpy(&netif->gateway, gateway ? gateway : ipaddr_get_empty(), sizeof(ipaddr_t));

    return NET_OK;
}

net_status_t netif_set_hwaddr(netif_t *netif, const char *hwaddr, int len) {

    plat_memcpy(netif->hwaddr.addr, hwaddr, len);

    netif->hwaddr.len = 0;
    return NET_OK;
}


net_status_t netif_set_active(netif_t *netif) {
    if (!netif) {
        return NET_ERROR_PARAM;
    }
    if (netif->state != NETIF_OPENED) {
        debug(DEBUG_WARNING, "netif is not opened");
        return NET_ERROR_STATE;
    }

    netif->state = NETIF_ACTIVE;
    if (!netif_default && (netif->type != NETIF_TYPE_LOOP)) {
        netif_set_default(netif);
    }

    display_netif_list();
    return NET_OK;
}

net_status_t netif_set_deactive(netif_t *netif) {
    if (!netif) {
        return NET_ERROR_PARAM;
    }
    if (netif->state != NETIF_ACTIVE) {
        debug(DEBUG_WARNING, "netif is not active");
        return NET_ERROR_STATE;
    }

    pkt_buf_t *buf;
    while ((buf = fix_queue_read_out(&netif->in_q, -1)) != (pkt_buf_t *) 0) {
        pkt_buf_free(buf);
    }

    while ((buf = fix_queue_read_out(&netif->out_q, -1)) != (pkt_buf_t *) 0) {
        pkt_buf_free(buf);
    }

    if (netif_default == netif) {
        netif_default = (netif_t *) 0;
    }
    netif->state = NETIF_OPENED;
    display_netif_list();
    return NET_OK;
}

net_status_t netif_close(netif_t *netif) {
    if (!netif) {
        return NET_ERROR_PARAM;
    }

    if (netif->state == NETIF_ACTIVE) {
        debug(DEBUG_ERROR, "netif is active.");
        return NET_ERROR_STATE;
    }

    netif->ops->close(netif);
    netif->state = NETIF_CLOSED;

    n_list_remove(&netif_list, &netif->node);
    m_block_free(&netif_block, netif);
    display_netif_list();

    return NET_OK;
}

void netif_set_default(netif_t *netif) {
    netif_default = netif;
}


net_status_t netif_put_in(netif_t *netif, pkt_buf_t *buf, int tmo) {
    net_status_t status = fix_queue_write_in(&netif->in_q, buf, tmo);
    if(status < NET_OK) {
        debug(DEBUG_WARNING, "netif in_q full");
        return NET_ERROR_FULL;
    }

    exmsg_netif_in(netif);
    return NET_OK;
}

pkt_buf_t *netif_get_in(netif_t *netif, int tmo) {
    pkt_buf_t *buf = fix_queue_read_out(&netif->in_q, tmo);

    if (buf) {
        pkt_buf_reset_acc(buf);
        return buf;
    }

    debug(DEBUG_WARNING, "netif in_q empty");
    return (pkt_buf_t *)0;
}

net_status_t netif_put_out(netif_t *netif, pkt_buf_t *buf, int tmo) {
    net_status_t status = fix_queue_write_in(&netif->out_q, buf, tmo);
    if(status < NET_OK) {
        debug(DEBUG_WARNING, "netif in_q full");
        return NET_ERROR_FULL;
    }

    return NET_OK;
}

pkt_buf_t *netif_get_out(netif_t *netif, int tmo) {
    pkt_buf_t *buf = fix_queue_read_out(&netif->out_q, tmo);

    if (buf) {
        pkt_buf_reset_acc(buf);
        return buf;
    }

    debug(DEBUG_WARNING, "netif out_q empty");
    return (pkt_buf_t *)0;
}

net_status_t netif_out(netif_t *netif, ipaddr_t *ipaddr, pkt_buf_t *buf) {
    net_status_t  status = netif_put_out(netif, buf, -1);

    if (status < NET_OK) {
        debug(DEBUG_WARNING, "send failed, queue full");
        return status;
    }

    return netif->ops->xmit(netif);
}