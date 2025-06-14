//
// Created by Administrator on 2025/5/21.
//
#include "netif_pcap.h"
#include "sys.h"
#include "ex_msg.h"
#include "ether.h"


static void rx_thread(void *arg) {
    plat_printf("receive thread is running ...\n");
    netif_t *netif = (netif_t *) arg;
    pcap_t *pcap = (pcap_t *) netif->ops_data;
    while (1) {
        struct pcap_pkthdr *pkthdr;
        const uint8_t *pkt_data;
        if (pcap_next_ex(pcap, &pkthdr, &pkt_data) != 1) {
            continue;
        }

        pkt_buf_t *buf = pkt_buf_alloc(pkthdr->len);
        if (buf == (pkt_buf_t *) 0) {
            debug(DEBUG_WARNING, "buf == nullptr");
            continue;
        }

        if (pkt_buf_write(buf, (uint8_t *) pkt_data, pkthdr->len) < NET_OK) {
            debug(DEBUG_WARNING, "netif: %s write error\n", netif->name);
            pkt_buf_free(buf);
            continue;
        }

        if (netif_put_in(netif, buf, 0) < 0) {
            debug(DEBUG_WARNING, "netif: %s in_q fill\n", netif->name);
            pkt_buf_free(buf);
            continue;
        }
    }
}

static void tx_thread(void *arg) {
    plat_printf("transmit thread is running ...\n");

    netif_t *netif = (netif_t *) arg;
    pcap_t *pcap = (pcap_t *) netif->ops_data;

    static uint8_t buffer[1500 + 6 + 6 + 2];
    while (1) {

        pkt_buf_t *buf = netif_get_out(netif, 0);

        if (buf == (pkt_buf_t *) 0) {
            continue;
        }

        plat_memset(buffer, 0, sizeof(buffer));
        int total_size = buf->total_size;
        pkt_buf_read(buf, buffer, total_size);
        pkt_buf_free(buf);

        if (pcap_inject(pcap, buffer, total_size) == -1) {
            debug(DEBUG_ERROR, "pcap send failed: %s", pcap_geterr(pcap));
            debug(DEBUG_ERROR, "pcap send failed, size: %d", total_size);
        }

    }
}

static net_status_t netif_pcap_open(netif_t *netif, void *data) {
    pcap_data_t *dev_data = (pcap_data_t *) data;

    pcap_t *pcap = pcap_device_open(dev_data->ip, dev_data->hwaddr);
    if (pcap == (pcap_t *) 0) {
        debug(DEBUG_ERROR, "pcp open failed, name : %s", netif->name);
        return NET_ERROR_IO;
    }

    netif->type = NETIF_TYPE_ETHER;
    netif->mtu = ETHER_MTU;
    netif->ops_data = pcap;
    netif_set_hwaddr(netif, dev_data->hwaddr, NETIF_HWADDR_SIZE);

    sys_thread_create(rx_thread, netif);
    sys_thread_create(tx_thread, netif);

    return NET_OK;
}

static void netif_pcap_close(netif_t *netif) {
    pcap_t *pcap = (pcap_t *) netif->ops_data;

    pcap_close(pcap);
}


static net_status_t netif_pcap_xmit(netif_t *netif) {



    return NET_OK;
}

const netif_ops_t net_dev_ops = {
        .open = netif_pcap_open,
        .close = netif_pcap_close,
        .xmit = netif_pcap_xmit,
};
