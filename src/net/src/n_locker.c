//
// Created by Administrator on 2025/5/24.
//
#include  "n_locker.h"

net_status_t n_locker_init(n_locker_t *locker, const n_locker_type_t type) {
    if (type == N_LOCKER_THREAD) {
        const sys_mutex_t mutex = sys_mutex_create();
        if (mutex == SYS_MUTEx_INVALID) {
            return NET_ERROR;
        }

        locker->mutex = mutex;
    }

    locker->type = type;

    return NET_OK;
}

void n_locker_destroy(const n_locker_t *locker) {
    if (locker->type == N_LOCKER_THREAD) {
        sys_mutex_free(locker->mutex);
    }
}

void n_locker_lock(const n_locker_t *locker) {
    if (locker->type == N_LOCKER_THREAD) {
        sys_mutex_lock(locker->mutex);
    }
}

void n_locker_unlock(const n_locker_t *locker) {
    if (locker->type == N_LOCKER_THREAD) {
        sys_mutex_unlock(locker->mutex);
    }
}
