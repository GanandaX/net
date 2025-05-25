//
// Created by Administrator on 2025/5/24.
//

#ifndef M_BLOCK_H
#define M_BLOCK_H

#include "n_list.h"
#include "n_locker.h"
#include "sys_plat.h"

typedef struct _m_block_t {
    n_list_t free_list;
    void *start;
    n_locker_t locker; // 对free链表进行读写保护
    sys_sem_t alloc_sem; // 控制资源数量
} m_block_t;

net_status_t m_block_init(m_block_t *m_block, void *mem, int block_size, int count, n_locker_type_t type);

void *m_block_alloc(const m_block_t *m_block, int ms);

int m_block_free_count(const m_block_t *m_block);

void m_block_free(const m_block_t *m_block, void *block);

void m_block_destroy(const m_block_t *m_block);
#endif //M_BLOCK_H
