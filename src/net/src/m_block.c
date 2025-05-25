//
// Created by Administrator on 2025/5/24.
//
#include "m_block.h"
#include "debug.h"

net_status_t m_block_init(m_block_t *m_block, void *mem,
                          const int block_size, const int count,
                          const n_locker_type_t type) {
    debug_assert(m_block != (void *)0, "block 为空");

    uint8_t *buffer = (uint8_t *) mem;
    n_list_init(&m_block->free_list);
    for (int i = 0; i < count; i++, buffer += block_size) {
        n_list_node_t *node = (n_list_node_t *) buffer;
        n_list_node_init(node);
        n_list_insert_last(&m_block->free_list, node);
    }

    m_block->start = mem;

    const net_status_t locker_status = n_locker_init(&m_block->locker, type);
    if (locker_status == NET_ERROR) {
        debug_error(DEBUG_M_BLOCK, "create locker failed.");
        return NET_ERROR;
    }

    if (type != N_LOCKER_NONE) {
        m_block->alloc_sem = sys_sem_create(count);

        if (m_block->alloc_sem == SYS_SEM_INVALID) {
            debug_error(DEBUG_M_BLOCK, "create sem failed.");
            n_locker_destroy(&m_block->locker);
            return NET_ERROR;
        }
    }
    return NET_OK;
}


void *m_block_alloc(const m_block_t *m_block, const int ms) {
    debug_assert(m_block != (void *)0, "block 为空");

    if (ms < 0 || m_block->locker.type == N_LOCKER_NONE) {
        n_locker_lock(&m_block->locker);
        const int count = m_block->free_list.count;

        if (count == 0) {
            n_locker_unlock(&m_block->locker);
            return (void *) 0;
        } else {
            n_list_node_t *block = n_list_remove_first(m_block);
            n_locker_unlock(&m_block->locker);
            return block;
        }
    } else {
        if (sys_sem_wait(m_block->alloc_sem, ms) < 0) {
            return (void *) 0;
        } else {
            n_locker_lock(&m_block->locker);
            n_list_node_t *block = n_list_remove_first(m_block);
            n_locker_unlock(&m_block->locker);
            return block;
        }
    }
}

int m_block_free_count(const m_block_t *m_block) {
    n_locker_lock(&m_block->locker);
    const int count = n_list_count(&m_block->free_list);
    n_locker_unlock(&m_block->locker);
    return count;
}

void m_block_free(const m_block_t *m_block, void *block) {
    n_locker_lock(&m_block->locker);
n_list_insert_last(&m_block->free_list, (n_list_node_t *)block);
    n_locker_unlock(&m_block->locker);

    if (m_block->locker.type != N_LOCKER_NONE) {
        sys_sem_notify(m_block->alloc_sem);
    }
}

void m_block_destroy(const m_block_t *m_block) {
    if (m_block->locker.type != N_LOCKER_NONE) {
        sys_sem_free(m_block->alloc_sem);
        n_locker_destroy(&m_block->locker);
    }
}
