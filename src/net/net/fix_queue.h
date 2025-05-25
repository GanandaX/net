//
// Created by Administrator on 2025/5/24.
//

#ifndef FIX_QUEUE_H
#define FIX_QUEUE_H

#include "n_locker.h"
#include "sys.h"

typedef struct _fix_queue_t {

    int size;
    int in_index;
    int out_index;
    int count;

    void **buf;

    n_locker_t locker;
    sys_sem_t write_sem;
    sys_sem_t read_sem;
}fix_queue_t;

net_status_t fix_queue_init(fix_queue_t *queue, void **buf, int size, n_locker_type_t locker_type);

net_status_t fix_queue_write_in(fix_queue_t *queue, void *msg, int tmo);

void *fix_queue_read_out(fix_queue_t *queue, int tmo);

void fix_queue_destroy(const fix_queue_t *queue);

int fix_queue_count(const fix_queue_t *queue);


#endif //FIX_QUEUE_H
