//
// Created by Administrator on 2025/5/21.
//
#include "netif_pcap.h"
#include "sys_plat.h"

static void rx_thread(void *arg) {
    plat_printf("receive thread is running ...\n");

    while (1) {
        sys_sleep(10);
    }
}

static void tx_thread(void *arg) {
    plat_printf("transmit thread is running ...\n");

    while (1) {
        sys_sleep(10);
    }
}

net_status_t netif_pcap_open(void) {
    sys_thread_create(rx_thread, (void *)0);
    sys_thread_create(tx_thread, (void *)0);

    return NET_OK;
}
