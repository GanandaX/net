//
// Created by Administrator on 2025/5/24.
//
#include "fix_queue.h"
#include "debug.h"

net_status_t fix_queue_init(fix_queue_t *queue, void **buf,
                            const int size, n_locker_type_t locker_type) {
    debug_assert(queue != (fix_queue_t *)0, "queue should not be null");

    queue->size = size;
    queue->in_index = queue->out_index = queue->count = 0;
    queue->buf = buf;
    queue->read_sem = SYS_SEM_INVALID;
    queue->write_sem = SYS_SEM_INVALID;

    net_status_t status = n_locker_init(&queue->locker, LOCKER_TYPE);
    if (status != NET_OK) {
        debug_error(DEBUG_QUEUE, "init locker failed.");
        return NET_ERROR;
    }

    queue->read_sem = sys_sem_create(0);
    queue->write_sem = sys_sem_create(size);

    if (queue->read_sem == SYS_SEM_INVALID || queue->write_sem == SYS_SEM_INVALID) {
        debug_error(DEBUG_QUEUE, "create sem failed.");
        status = NET_ERROR;
        goto init_failed;
    }

    return NET_OK;
init_failed:
    if (queue->read_sem != SYS_SEM_INVALID) {
        sys_sem_free(queue->read_sem);
    }
    if (queue->write_sem != SYS_SEM_INVALID) {
        sys_sem_free(queue->write_sem);
    }

    n_locker_destroy(&queue->locker);
    return status;
}

net_status_t fix_queue_write_in(fix_queue_t *queue, void *msg, int tmo) {
    debug_assert(queue != (fix_queue_t *)0, "queue should not be null");

    n_locker_lock(&queue->locker);
    if (tmo < 0 && queue->count >= queue->size) {
        n_locker_unlock(&queue->locker);
        return NET_ERROR_FULL;
    }
    n_locker_unlock(&queue->locker);

    if (sys_sem_wait(queue->write_sem, tmo) < 0) {
        return NET_ERROR_TIMEOUT;
    }

    n_locker_lock(&queue->locker);
    queue->buf[queue->in_index++] = msg;
    queue->in_index %= queue->size;
    queue->count++;
    n_locker_unlock(&queue->locker);

    sys_sem_notify(queue->read_sem);

    return NET_OK;
}

void *fix_queue_read_out(fix_queue_t *queue, int tmo) {
    n_locker_lock(&queue->locker);

    if (tmo < 0 && !queue->count) {
        n_locker_unlock(&queue->locker);
        return (void *) 0;
    }
    n_locker_unlock(&queue->locker);

    if (sys_sem_wait(queue->read_sem, tmo) < 0) {
        return (void *) 0;
    }

    n_locker_lock(&queue->locker);
    void *msg = queue->buf[queue->out_index++];
    queue->out_index %= queue->size;
    queue->count--;
    n_locker_unlock(&queue->locker);

    return msg;
}

void fix_queue_destroy(const fix_queue_t *queue) {
    n_locker_destroy(&queue->locker);
    sys_sem_free(queue->read_sem);
    sys_sem_free(queue->write_sem);
}

int fix_queue_count(const fix_queue_t *queue) {
    n_locker_lock(&queue->locker);
    const int count = queue->count;
    n_locker_unlock(&queue->locker);

    return count;
}
