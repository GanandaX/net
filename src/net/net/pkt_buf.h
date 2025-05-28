//
// Created by Administrator on 2025/5/25.
//

#ifndef PKT_BUF_H
#define PKT_BUF_H

#include "n_list.h"
#include "stdint.h"
#include "net_config.h"
#include "net_status.h"

typedef struct _pkt_blk_t {
    n_list_node_t node;
    int size;
    uint8_t *data;
    uint8_t payload[PKT_BLK_SIZE];
} pkt_blk_t;


typedef struct _pkt_buf_t {
    n_list_t list;
    n_list_node_t node;
    int total_size;

    int pos;
    pkt_blk_t *cur_blk;
    uint8_t *blk_offset;

    int ref;
} pkt_buf_t;

typedef enum _data_insert_mod {
    Header_Insertion,
    Tail_Insertion,
} data_insert_mod;

typedef enum _add_header_mod {
    CONTINUE,
    DISCONTINUE,
} add_header_mod;

net_status_t pkt_buf_init(void);

pkt_buf_t *pkt_buf_alloc(int size);

void pkt_buf_insert_blk_list(pkt_buf_t *buf, pkt_blk_t *first_blk, data_insert_mod insert_mod);

static inline pkt_blk_t *pkt_blk_next(pkt_blk_t *pkt_blk) {
    return n_list_entity(n_list_node_next(&pkt_blk->node), node, pkt_blk_t);
}

static inline pkt_blk_t *pkt_buf_first_blk(pkt_buf_t *buf) {
    return n_list_entity(n_list_first(&buf->list), node, pkt_blk_t);
}

static inline pkt_blk_t *pkt_buf_last_blk(pkt_buf_t *buf) {
    return n_list_entity(n_list_last(&buf->list), node, pkt_blk_t);
}

void pkt_buf_free(pkt_buf_t *buf);

/**
 *
 * @param buf
 * @param size
 * @param add_mod 数据在数据块是否是连续的
 * @return
 */
net_status_t pkt_add_header(pkt_buf_t *buf, int size, add_header_mod add_mod);

net_status_t pkt_remove_header(pkt_buf_t *buf, int size);

net_status_t pkt_buf_resize(pkt_buf_t *buf, int size);

net_status_t pkt_buf_join(pkt_buf_t *dest, pkt_buf_t *src);

net_status_t pkt_buf_set_continue_space(pkt_buf_t *buf, int size);

void pkt_buf_reset_acc(pkt_buf_t *buf);

net_status_t pkt_buf_write(pkt_buf_t *buf, uint8_t *data, int size);

net_status_t pkt_buf_read(pkt_buf_t *buf, uint8_t *data, int size);

static int inline pkt_buf_total(pkt_buf_t *buf) {
    return buf->total_size;
}

net_status_t pkt_buf_seek(pkt_buf_t *buf, int offset);

net_status_t pkt_buf_copy(pkt_buf_t *dest, pkt_buf_t *src, int size);

net_status_t pkt_buf_fill(pkt_buf_t *buf, uint8_t val, int size);

void pkt_buf_inc_ref(pkt_buf_t *buf);

#endif //PKT_BUF_H
