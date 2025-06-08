//
// Created by Administrator on 2025/5/21.
//
#include "ex_msg.h"

#include "debug.h"
#include "sys_plat.h"
#include "fix_queue.h"
#include "m_block.h"

// msg_block 存放具体信息, msg_queue表示消息队列
static void *msg_table[EXMSG_MSG_CNT];
static fix_queue_t msg_queue;
static exmsg_t msg_buff[EXMSG_MSG_CNT];
static m_block_t msg_block;

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

static void work_thread(void *arg) {
    debug_info(DEBUG_MSG, "ex_msg is running ...");

    while (1) {
        exmsg_t *msg = (exmsg_t *) fix_queue_read_out(&msg_queue, 0);
        debug_info(DEBUG_MSG, "recving a msg type: %d, id: %d", msg->type, msg->id);
        m_block_free(&msg_block, msg);
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

    static int id = 0;
    msg->type = NET_EXMSG_NETIF_IN;
    msg->id = id++;

    const net_status_t status = fix_queue_write_in(&msg_queue, msg, -1);
    if (status != NET_OK) {
        debug_warning(DEBUG_MSG, "fix queue write failed may be full");
        m_block_free(&msg_block, msg);
    }

    return status;
}
