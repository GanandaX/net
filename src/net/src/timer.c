//
// Created by GrandBand on 2025/6/14.
//
#include "timer.h"
#include "debug.h"
#include "net_plat.h"

static n_list_t timer_list;


#if DEBUG_DISP_ENABLED(DEBUG_TIMER)

static void display_timer_list() {
    plat_printf("----------%s----------\n", "timer list");

    n_list_node_t *node;
    uint32_t index = 0;
    n_list_for_each(node, &timer_list) {

        net_timer_t *timer = n_list_entity(node, node, net_timer_t);

        plat_printf("%d: %s, counter= %d ms, reload: %d ms, period: %s, added: %s\n", index++, timer->name,
                    timer->counter,
                    timer->reload, (timer->flags & NET_TIMER_RELOAD) ? "true" : "false",
                    (timer->flags & NET_TIMER_ADDED) ? "true" : "false");
    }

    plat_printf("----------%s----------\n", "timer list end");
}

#else
#define display_timer_list()
#endif


net_status_t net_timer_init(void) {
    debug(DEBUG_INFO, "timer init.");

    n_list_init(&timer_list);


    debug(DEBUG_INFO, "timer init done.");
    return NET_OK;
}

static void insert_timer(net_timer_t *insert_timer) {
    n_list_node_t *node;
    insert_timer->flags = insert_timer->flags | NET_TIMER_ADDED;

    n_list_for_each(node, &timer_list) {
        net_timer_t *cur_timer = n_list_entity(node, node, net_timer_t);

        if (insert_timer->counter > cur_timer->counter) {
            insert_timer->counter -= cur_timer->counter;
        } else if (insert_timer->counter == cur_timer->counter) {
            insert_timer->counter = 0;
            n_list_insert_after(&timer_list, node, &insert_timer->node);
            return;
        } else {
            cur_timer->counter -= insert_timer->counter;

            n_list_node_t *pre = n_list_node_pre(&cur_timer->node);
            if (pre) {
                n_list_insert_after(&timer_list, pre, &insert_timer->node);
            } else {
                n_list_insert_first(&timer_list, &insert_timer->node);
            }
            return;
        }
    }

    n_list_insert_last(&timer_list, &insert_timer->node);
}

net_status_t net_timer_add(net_timer_t *timer, const char *name,
                           timer_proc_t proc, void *arg,
                           uint32_t ms, uint32_t flags) {

    debug(DEBUG_INFO, "insert timer: %s", name);

    plat_strncpy(timer->name, name, TIMER_NAME_SIZE);

    timer->name[TIMER_NAME_SIZE - 1] = 0;
    timer->reload = ms;
    timer->counter = ms;
    timer->proc = proc;
    timer->arg = arg;
    timer->flags = flags;

    insert_timer(timer);

    display_timer_list();
    return NET_OK;
}

net_status_t net_timer_remove(net_timer_t *timer) {
    debug(DEBUG_INFO, "remove timer: %s", timer->name);

    if (!(timer->flags = timer->flags & NET_TIMER_ADDED)) {
        debug(DEBUG_WARNING, "this timer(%s) was not added to the list.", timer->name);
        return NET_ERROR_NOT_EXIST;
    }

    n_list_node_t *next_node = n_list_node_next(&timer->node);
    if (next_node) {
        net_timer_t *next_timer = n_list_entity(next_node, node, net_timer_t);
        next_timer->counter += timer->counter;
    }

    timer->flags = timer->flags & ~NET_TIMER_ADDED;
    n_list_remove(&timer_list, &timer->node);

    display_timer_list();
}


net_status_t net_timer_check_tmo(int interval) {

    n_list_t wait_list;
    n_list_init(&wait_list);

    n_list_node_t *node = n_list_first(&timer_list);
    while (node) {
        n_list_node_t *next_node = n_list_node_next(node);
        net_timer_t *timer = n_list_entity(node, node, net_timer_t);

        if (timer->counter > interval) {
            timer->counter -= interval;
            break;
        }

        interval -= timer->counter;
        timer->counter = 0;

        n_list_remove(&timer_list, &timer->node);
        n_list_insert_last(&wait_list, &timer->node);

        node = next_node;
    }

    display_timer_list();

    while ((node = n_list_remove_first(&wait_list)) != (n_list_node_t *)0) {
        net_timer_t  *timer = n_list_entity(node, node, net_timer_t);

        timer->proc(timer, timer->arg);

        if (timer->flags & NET_TIMER_RELOAD) {
            timer->counter = timer->reload;
            insert_timer(timer);
        }
    }

    display_timer_list();
    return NET_OK;
}

uint32_t net_timer_first_tmo(void) {
    n_list_node_t  *node = n_list_first(&timer_list);

    if (node) {
        net_timer_t *timer = n_list_entity(node, node, net_timer_t);
        return timer->counter;
    }

    return 0;
}