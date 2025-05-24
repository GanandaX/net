/**
 * @file main.c
 * @author lishutong (527676163@qq.com)
 * @brief 测试主程序，完成一些简单的测试主程序
 * @version 0.1
 * @date 2022-10-23
 *
 * @copyright Copyright (c) 2022
 * @note 该源码配套相应的视频课程，请见源码仓库下面的README.md
 */
#include <stdio.h>
#include "sys_plat.h"
#include "windows.h"
#include "echo/tcp_echo_client.h"
#include "echo/tcp_echo_server.h"

#include "net.h"
#include "netif_pcap.h"
#include "debug.h"
#include "n_list.h"

sys_sem_t sem;
int count;
sys_mutex_t mutex;

unsigned char buffer[100];
sys_sem_t read_sem;
sys_sem_t write_sem;
unsigned char read_index;
unsigned char write_index;
unsigned char total_count;

void thread1_entry(void *arg) {

    // 读线程
    while (1) {

        sys_sem_wait(read_sem, 0);

        unsigned char data = buffer[read_index++];
        plat_printf("read thread: %d\n", data);

        sys_mutex_lock(mutex);
        total_count--;
        sys_mutex_unlock(mutex);

        sys_sem_notify(write_sem);
        sys_sleep(200);
    }

    /* for (size_t i = 0; i < 100; i++)
     {
         sys_mutex_lock(mutex);
         count++;
         plat_printf("thread 1: %d\n", count);
         sys_mutex_unlock(mutex);
     }*/

    /*  while(1) {
         plat_printf("thread1: %s\n", (char*)arg);
         sys_sem_notify(sem);
         sys_sleep(1000);
     } */
}

void thread2_entry(void *arg) {

    unsigned char number = 1;
    // 写线程
    while (1) {

        sys_sem_wait(write_sem, 0);

        buffer[write_index++] = number++;
        plat_printf("write thread: %d\n", number - 1);

        sys_mutex_lock(mutex);
        total_count++;
        sys_mutex_unlock(mutex);

        sys_sem_notify(read_sem);
        sys_sleep(100);
    }

    /*sys_sem_wait(mutex, 0);

    for (size_t i = 0; i < 100; i++)
    {
        sys_mutex_lock(mutex);
        count--;
        plat_printf("thread 2: %d\n", count);
        sys_mutex_unlock(mutex);
    }*/
    /*  while(1) {
         sys_sem_wait(sem, 0);
         plat_printf("thread2: %s\n", (char*)arg);
     } */
}

net_status_t net_dev_init() {
    netif_pcap_open();

    return NET_OK;
}


typedef struct _t_node_t {
    int id;
    n_list_node_t node;
} t_node_t;


void travel_list_test(n_list_node_t *node) {
    t_node_t *t_node = n_list_entity(node, node, t_node_t);
//    t_node_t  *t_node = (t_node_t *)((char *)node - (char *)(&(((t_node_t*)0))->node));
    debug_info(DEBUG_INFO, "%d\n", t_node->id);
}

void n_list_test() {
#define NODE_CNT 4
    t_node_t node[NODE_CNT];
    n_list_t list;

    debug_info(DEBUG_INFO, "first");
    n_list_init(&list);
    for (int i = 0; i < NODE_CNT; ++i) {
        node[i].id = i;
        n_list_insert_first(&list, &node[i].node);
    }
    n_list_travel(&list, travel_list_test);

    for (int i = 0; i < NODE_CNT; ++i) {
        n_list_node_t *n_node = n_list_remove_first(&list);
        if (n_node == (n_list_node_t *) 0) {
            debug_error(DEBUG_ERROR, "node 删除错误");
        }

        debug_info(DEBUG_INFO, "id: %d  count: %d", n_list_entity(n_node, node, t_node_t)->id, list.count);
    }


    debug_info(DEBUG_INFO, "\n\nsecond\n\n");
    n_list_init(&list);
    for (int i = 0; i < NODE_CNT; ++i) {
        node[i].id = i;
        n_list_insert_last(&list, &node[i].node);
    }

    debug_info(DEBUG_INFO, "\n");
    n_list_travel(&list, travel_list_test);
    debug_info(DEBUG_INFO, "\n");

    for (int i = 0; i < NODE_CNT; ++i) {
        n_list_node_t *n_node = n_list_remove_last(&list);
        if (n_node == (n_list_node_t *) 0) {
            debug_error(DEBUG_ERROR, "node 删除错误");
        }

        debug_info(DEBUG_INFO, "id: %d  count: %d", n_list_entity(n_node, node, t_node_t)->id, list.count);
    }


    debug_info(DEBUG_INFO, "\n\nthird\n\n");
    n_list_init(&list);
    for (int i = 0; i < NODE_CNT; ++i) {
        node[i].id = i;
        n_list_insert_after(&list, n_list_first(&list), &node[i].node);
    }

    debug_info(DEBUG_INFO, "\n");
    n_list_travel(&list, travel_list_test);
    debug_info(DEBUG_INFO, "\n");

   /* for (int i = 0; i < NODE_CNT; ++i) {
        n_list_node_t *n_node = n_list_remove_last(&list);
        if (n_node == (n_list_node_t *) 0) {
            debug_error(DEBUG_ERROR, "node 删除错误");
        }

        debug_info(DEBUG_INFO, "id: %d  count: %d", n_list_entity(n_node, node, t_node_t)->id, list.count);
    }*/
}

void base_test() {
    n_list_test();
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);  // 设置 stdout 为无缓冲

    /*  plat_printf("*** INFO ***\n");
      debug_info(DEBUG_INFO, "json1");
      debug_info(DEBUG_WARNING, "json2");
      debug_info(DEBUG_ERROR, "json3");

      plat_printf("*** WARNING ***\n");
      debug_warning(DEBUG_INFO, "json1");
      debug_warning(DEBUG_WARNING, "json2");
      debug_warning(DEBUG_ERROR, "json3");

      plat_printf("*** ERROR ***\n");
      debug_error(DEBUG_INFO, "json1");
      debug_error(DEBUG_WARNING, "json2");
      debug_error(DEBUG_ERROR, "json3");*/


//    debug_assert(3 == 0, "3不等于0");


    base_test();

    net_init();
    net_start();
    net_dev_init();

    while (1) {
        sys_sleep(10);
    }
    return 0;
}