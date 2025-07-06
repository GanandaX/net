//
// Created by Administrator on 2025/5/21.
//
#include "ex_msg.h"

#include "debug.h"
#include "sys_plat.h"
#include "fix_queue.h"
#include "m_block.h"
#include "timer.h"
#include "ipv4.h"

// msg_block 存放具体信息, msg_queue表示消息队列
static void *msg_table[EXMSG_MSG_CNT];
static fix_queue_t msg_queue;
static exmsg_t msg_buff[EXMSG_MSG_CNT];
static m_block_t msg_block;

net_status_t test_func(func_msg_t *msg) {
    printf("hello, 1234: 0X%x\n", *(int *) msg->param);

    return NET_OK;
}

net_status_t ex_msg_init() {
    debug_info(DEBUG_MSG, "exmsg init");
    net_status_t status = fix_queue_init(&msg_queue, msg_table,
                                         EXMSG_MSG_CNT, LOCKER_TYPE);

    if (status != NET_OK) {
        debug_error(DEBUG_MSG, "fix queue init failed");
        return status;
    }

    status = m_block_init(&msg_block, msg_buff,
                          sizeof(exmsg_t), EXMSG_MSG_CNT, LOCKER_TYPE);
    if (status != NET_OK) {
        debug_error(DEBUG_MSG, "fix queue init failed");
        return status;
    }

    debug_info(DEBUG_MSG, "exmsg init done.");

    return NET_OK;
}

static net_status_t do_netif_in(exmsg_t *msg) {
    netif_t *netif = msg->netif.netif;

    pkt_buf_t *buf;
    while (buf = netif_get_in(netif, -1)) {
        debug(DEBUG_INFO, "recv a packet");
        net_status_t status;
        if (netif->link_layer) {
            status = netif->link_layer->in(netif, buf);
            if (status < NET_OK) {
                debug(DEBUG_WARNING, "netif in failed, status= %d", status);
                pkt_buf_free(buf);
            }
        } else {
            status = ipv4_in(netif, buf);
            if (status < NET_OK) {
                debug(DEBUG_WARNING, "netif in failed, status= %d", status);
                pkt_buf_free(buf);
            }
        }
    }

    return NET_OK;
}

static net_status_t do_func(exmsg_t *msg) {
    func_msg_t *func_msg = msg->func;

    debug(DEBUG_INFO, "call func");
    func_msg->status = func_msg->func(func_msg);
    sys_sem_notify(func_msg->wait_sem);
    debug(DEBUG_INFO, "call func done");

    return NET_OK;
}

static void work_thread(void *arg) {
    debug_info(DEBUG_MSG, "ex_msg is running ...");

    net_time_t time;
    sys_time_curr(&time);

    while (1) {
        int first_tmo = net_timer_first_tmo();
        exmsg_t *msg = (exmsg_t *) fix_queue_read_out(&msg_queue, first_tmo);

        if (msg) {
            debug(DEBUG_INFO, "recv a msg %p: %d", msg, msg->type);
            switch (msg->type) {
                case NET_EXMSG_NETIF_IN:
                    do_netif_in(msg);
                    break;
                case NET_EXMSG_FUN:
                    do_func(msg);
                    break;
                default:
                    break;
            }

            m_block_free(&msg_block, msg);
        }

        uint32_t interval = sys_time_goes(&time);
        net_timer_check_tmo(interval);
    }
}

net_status_t ex_msg_start() {
    sys_thread_t thread = sys_thread_create(work_thread, (void *) 0);

    if (thread == SYS_THREAD_INVALID) {
        return NET_ERROR;
    }

    return NET_OK;
}

net_status_t exmsg_netif_in(netif_t *netif) {
    exmsg_t *msg = m_block_alloc(&msg_block, -1);

    if (!msg) {
        debug_error(DEBUG_MSG, "exmsg alloc failed");
        return NET_ERROR_MEM;
    }

    msg->type = NET_EXMSG_NETIF_IN;
    msg->netif.netif = netif;

    const net_status_t status = fix_queue_write_in(&msg_queue, msg, -1);
    if (status != NET_OK) {
        debug_warning(DEBUG_MSG, "fix queue write failed may be full");
        m_block_free(&msg_block, msg);
        return status;
    }

    return status;
}

net_status_t exmsg_func_exec(exmsg_func_t func, void *param) {
    func_msg_t func_msg;
    func_msg.func = func;
    func_msg.param = param;
    func_msg.status = NET_OK;
    func_msg.thread = sys_thread_self();
    func_msg.wait_sem = sys_sem_create(0);
    if (func_msg.wait_sem == SYS_SEM_INVALID) {
        debug(DEBUG_WARNING, "exmsg wait sem create failed");
        return NET_ERROR_MEM;
    }

    exmsg_t *msg = m_block_alloc(&msg_block, 0);
    if (!msg) {
        debug_error(DEBUG_MSG, "exmsg alloc failed");
        sys_sem_free(func_msg.wait_sem);
        return NET_ERROR_MEM;
    }
    msg->type = NET_EXMSG_FUN;
    msg->func = &func_msg;

    debug(DEBUG_INFO, "exmsg exec func start: %p", func);

    const net_status_t status = fix_queue_write_in(&msg_queue, msg, 0);
    if (status != NET_OK) {
        debug_warning(DEBUG_MSG, "fix queue write failed may be full");
        sys_sem_free(func_msg.wait_sem);
        m_block_free(&msg_block, msg);
        return status;
    }

    sys_sem_wait(func_msg.wait_sem, 0);
    debug(DEBUG_INFO, "exmsg exec func done. status: %d", func_msg.status);
    return func_msg.status;
}
