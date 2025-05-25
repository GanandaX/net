//
// Created by Administrator on 2025/5/24.
//

#ifndef N_LOCKER_H
#define N_LOCKER_H

#include "sys_plat.h"
#include "net_status.h"

typedef enum _n_locker_type_t {
    N_LOCKER_NONE,
    N_LOCKER_THREAD,
} n_locker_type_t;

typedef struct _n_locker_t {
    n_locker_type_t type;

    union {
        sys_mutex_t mutex;
    };
} n_locker_t;

net_status_t n_locker_init(n_locker_t *locker, n_locker_type_t type);

void n_locker_destroy(const n_locker_t *locker);

void n_locker_lock(const n_locker_t *locker);

void n_locker_unlock(const n_locker_t *locker);


#endif //N_LOCKER_H
