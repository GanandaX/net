//
// Created by Administrator on 2025/5/21.
//
#include "ex_msg.h"
#include "sys_plat.h"

net_status_t ex_msg_init() {
    return NET_OK;
}

static void work_thread(void *arg) {
    plat_printf("ex_msg is running ...\n");

    while (1) {
        sys_sleep(10);
    }
}

net_status_t ex_msg_start() {

    sys_thread_t thread = sys_thread_create(work_thread, (void *) 0);

    if (thread == SYS_THREAD_INVALID) {
        return NET_ERROR;
    }

    return NET_OK;
}