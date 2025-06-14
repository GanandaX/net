//
// Created by Administrator on 2025/5/25.
//
#include "pkt_buf.h"
#include "m_block.h"
#include "n_locker.h"

static n_locker_t locker;
static pkt_blk_t block_buffer[PKT_BLK_CNT];
static m_block_t block_list;
static pkt_buf_t pkt_buf_buffer[PKT_BUF_CNT];
static m_block_t pkt_buf_list;

static int pkt_current_blk_remain_size(pkt_buf_t *buf) {
    if (!buf->cur_blk) {
        return 0;
    }
    return (int) (buf->cur_blk->data + buf->cur_blk->size - buf->blk_offset);
}

static int total_blk_remain_relate_to_pos(pkt_buf_t *buf) {
    return buf->total_size - buf->pos;
}

static int current_blk_tail_left(pkt_blk_t *blk) {
    return (int) (blk->payload + PKT_BLK_SIZE - (blk->data + blk->size));
}


#if DEBUG_DISP_ENABLED(DEBUG_BUF)
static void display_check_buf(pkt_buf_t *buf) {
    if (!buf) {
        debug(DEBUG_ERROR, "invalid buf, buf == 0");
        return;
    }

    debug(DEBUG_INFO, "check buf: %p    size: %d", buf, buf->total_size);
    int blk_index = 0;
    int total_size = 0;
    for (pkt_blk_t *blk = pkt_buf_first_blk(buf); blk; blk = pkt_blk_next(blk)) {
        plat_printf("%d: ", blk_index++);
        if (blk->data < blk->payload || blk->data > blk->payload + PKT_BLK_SIZE) {
            debug(DEBUG_ERROR, "invalid data point, blk_index: %d", blk_index);
        }

        plat_printf("pre_size: %d  used: %d   left: %d\n", blk->data - blk->payload, blk->size, current_blk_tail_left(blk));

        const int blk_size = (blk->data - blk->payload + blk->size + current_blk_tail_left(blk));
        if (blk_size != PKT_BLK_SIZE) {
            debug(DEBUG_ERROR, "bad blk size: %d != %d", blk_size, PKT_BLK_SIZE);
        }

        total_size += blk->size;
    }

    if (total_size != buf->total_size) {
        debug(DEBUG_ERROR, "bad buf size: %d != %d", total_size, buf->total_size);
    }
}
#else
#define display_check_buf(buf)
#endif


net_status_t pkt_buf_init(void) {
    debug(DEBUG_BUF, "init pkt buf");

    n_locker_init(&locker, N_LOCKER_THREAD);
    m_block_init(&block_list, block_buffer, sizeof(pkt_blk_t), PKT_BLK_CNT, N_LOCKER_NONE);
    m_block_init(&pkt_buf_list, pkt_buf_buffer, sizeof(pkt_buf_t), PKT_BUF_CNT, N_LOCKER_NONE);

    debug(DEBUG_BUF, "init pkt ok");
    return NET_OK;
}

static pkt_blk_t *pkt_blk_alloc(void) {
    n_locker_lock(&locker);
    pkt_blk_t *block = m_block_alloc(&block_list, -1);
    n_locker_unlock(&locker);
    if (block) {
        block->size = 0;
        block->data = (uint8_t *) 0;
        n_list_node_init(&block->node);
    }


    return block;
}

static void pkt_blk_free(pkt_blk_t *blk) {
    n_locker_lock(&locker);
    m_block_free(&block_list, blk);
    n_locker_unlock(&locker);
}

static void pkt_blk_list_free(pkt_blk_t *blk) {
    while (blk) {
        pkt_blk_t *next = pkt_blk_next(blk);
        pkt_blk_free(blk);
        blk = next;
    }
}

static pkt_blk_t *pkt_block_alloc_list(int size, const data_insert_mod insert_mod) {
    pkt_blk_t *first_block = (pkt_blk_t *) 0;
    pkt_blk_t *pre_block = (pkt_blk_t *) 0;
    while (size) {
        pkt_blk_t *new_block = pkt_blk_alloc();

        if (!new_block) {
            debug(DEBUG_BUF, "no buffer for alloc block(%d)", size);
            if (first_block) {
                pkt_blk_list_free(first_block);
            }
            return (pkt_blk_t *) 0;
        }

        const int cur_size = size > PKT_BLK_SIZE ? PKT_BLK_SIZE : size;
        size -= cur_size;
        if (insert_mod == Header_Insertion) {
            if (pre_block) {
                n_list_node_set_pre(&pre_block->node, &new_block->node);
            }
            new_block->size = cur_size;
            new_block->data = new_block->payload + PKT_BLK_SIZE - cur_size;

            first_block = new_block;
        } else {
            if (!pre_block) {
                first_block = new_block;
            } else {
                n_list_node_set_next(&pre_block->node, &new_block->node);
            }

            new_block->size = cur_size;
            new_block->data = new_block->payload;
        }

        pre_block = new_block;
    }

    return first_block;
}

pkt_buf_t *pkt_buf_alloc(const int size) {
    // pkt_block_alloc_list(size, Head_Insertion);
    // pkt_block_alloc_list(size, Tail_Insertion);
    if (size <= 0) {
        return (pkt_buf_t *) 0;
    }

    n_locker_lock(&locker);
    pkt_buf_t *buf = m_block_alloc(&pkt_buf_list, -1);
    n_locker_unlock(&locker);
    if (!buf) {
        debug(DEBUG_BUF, "no buffer for alloc buf");
        return (pkt_buf_t *) 0;
    }

    buf->total_size = 0;
    buf->ref = 1;
    n_list_init(&buf->list);
    n_list_node_init(&buf->node);

    pkt_blk_t *block = pkt_block_alloc_list(size, Header_Insertion);
    if (!block) {
        debug(DEBUG_BUF, "no buffer for alloc block");
        pkt_blk_list_free(block);
        pkt_buf_free(buf);

        return (pkt_buf_t *) 0;
    }

    pkt_buf_insert_blk_list(buf, block, Header_Insertion);
    pkt_buf_reset_acc(buf);

    display_check_buf(buf);
    return buf;
}

void pkt_buf_insert_blk_list(pkt_buf_t *buf, pkt_blk_t *first_blk, data_insert_mod insert_mod) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (insert_mod == Tail_Insertion) {
        while (first_blk) {
            pkt_blk_t *next_blk = pkt_blk_next(first_blk);

            n_list_insert_last(&buf->list, &first_blk->node);
            buf->total_size += first_blk->size;

            first_blk = next_blk;
        }
    } else if (insert_mod == Header_Insertion) {
        // 这个是将first_blk这个链表插入到buf的整体前面
        pkt_blk_t *pre = (void *) 0; // first_blk 假如为1->2->3    buf为4->5->6
        while (first_blk) {
            // 插入后     1->2->3->4->5->6
            pkt_blk_t *next_blk = pkt_blk_next(first_blk);

            if (pre) {
                n_list_insert_after(&buf->list, &pre->node, &first_blk->node);
            } else {
                n_list_insert_first(&buf->list, &first_blk->node);
            }

            pre = first_blk;
            buf->total_size += first_blk->size;

            first_blk = next_blk;
        }
    }
}


void pkt_buf_free(pkt_buf_t *buf) {
    debug_assert(buf->ref != 0, "buf->ref == 0");

    n_locker_lock(&locker);
    if (--buf->ref == 0) {
        pkt_blk_list_free(pkt_buf_first_blk(buf));
        n_locker_lock(&locker);
        m_block_free(&pkt_buf_list, buf);
        n_locker_unlock(&locker);
    }
    n_locker_unlock(&locker);
}


net_status_t pkt_buf_add_header(pkt_buf_t *buf, int size, add_header_mod add_mod) {
    debug_assert(buf->ref != 0, "buf->ref == 0");

    pkt_blk_t *block = pkt_buf_first_blk(buf);

    const int header_reserve = (int) (block->data - block->payload);
    if (header_reserve >= size) {
        block->data -= size;
        block->size += size;
        buf->total_size += size;

        display_check_buf(buf);
        return NET_OK;
    }

    if (add_mod == DISCONTINUE) {
        block->data = block->payload;
        block->size += header_reserve;
        buf->total_size += header_reserve;
        size -= header_reserve;

        block = pkt_block_alloc_list(size, Header_Insertion);
        if (!block) {
            debug(DEBUG_NO_FREE_BLCK, "no block");
            return NET_ERROR_MEM;
        }
    } else if (add_mod == CONTINUE) {
        if (size > PKT_BLK_SIZE) {
            debug(DEBUG_BUF, "size too big %d < %d", PKT_BLK_SIZE, size);
            return NET_ERROR_SIZE;
        }

        block = pkt_block_alloc_list(size, Header_Insertion);
        if (!block) {
            debug(DEBUG_BUF, "no buffer for alloc block %d", size);
            return NET_ERROR_NO_SOURCE;
        }
    }

    pkt_buf_insert_blk_list(buf, block, Header_Insertion);
    display_check_buf(buf);

    return NET_OK;
}

net_status_t pkt_remove_header(pkt_buf_t *buf, int size) {
    debug_assert(buf->total_size >= size, "包大小 < 删除大小");
    debug_assert(buf->ref != 0, "buf->ref == 0");

    pkt_blk_t *blk = pkt_buf_first_blk(buf);

    if (blk->size > size) {
        blk->size -= size;
        blk->data += size;
        buf->total_size -= size;
    } else {
        while (blk->size <= size) {
            buf->total_size -= blk->size;
            size -= blk->size;
            n_list_remove_first(&buf->list);
            pkt_blk_free(blk);
            blk = pkt_buf_first_blk(buf);
        }

        if (size) {
            blk->size -= size;
            blk->data += size;
            buf->total_size -= size;
        }
    }

    display_check_buf(buf);
    return NET_OK;
}

net_status_t pkt_buf_resize(pkt_buf_t *buf, int size) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (size == buf->total_size) {
        return NET_OK;
    }

    if (buf->total_size == 0) {
        pkt_blk_t *blk_list = pkt_block_alloc_list(size, Header_Insertion);
        if (!blk_list) {
            debug(DEBUG_NO_FREE_BLCK, "no free block");
            return NET_ERROR_MEM;
        }

        pkt_buf_insert_blk_list(buf, blk_list, Header_Insertion);
    } else if (size > buf->total_size) {
        pkt_blk_t *tail_blk = pkt_buf_last_blk(buf);
        int inc_size = size - buf->total_size;
        int blk_ramain_size = current_blk_tail_left(tail_blk);
        if (blk_ramain_size >= inc_size) {
            tail_blk->size += inc_size;
            buf->total_size += inc_size;
        } else {
            pkt_blk_t *blk_list = pkt_block_alloc_list(inc_size - blk_ramain_size, Tail_Insertion);
            if (!blk_list) {
                debug(DEBUG_NO_FREE_BLCK, "no free block");
                return NET_ERROR_MEM;
            }

            tail_blk->size += blk_ramain_size;
            buf->total_size += blk_ramain_size;
            pkt_buf_insert_blk_list(buf, blk_list, Tail_Insertion);
        }
    } else {
        pkt_blk_t *tail_blk = pkt_buf_last_blk(buf);
        int remove_size = buf->total_size - size;

        while (remove_size) {
            if (remove_size < tail_blk->size) {
                tail_blk->size -= remove_size;
                buf->total_size -= remove_size;
                break;
            }
            buf->total_size -= tail_blk->size;
            remove_size -= tail_blk->size;
            n_list_remove_last(&buf->list);
            pkt_blk_free(tail_blk);
            tail_blk = pkt_buf_last_blk(buf);
        }

        if (!n_list_count(&buf->list)) {
            n_list_init(&buf->list);
        }
    }


    display_check_buf(buf);

    return NET_OK;
}

net_status_t pkt_buf_join(pkt_buf_t *dest, pkt_buf_t *src) {
    debug_assert(dest->ref != 0, "dest->ref == 0");
    debug_assert(src->ref != 0, "src->ref == 0");
    pkt_blk_t *blk;
    while ((blk = pkt_buf_first_blk(src))) {
        n_list_remove_first(&src->list);
        pkt_buf_insert_blk_list(dest, blk, Tail_Insertion);
    }

    pkt_buf_free(src);
    display_check_buf(dest);
    return NET_OK;
}

net_status_t pkt_buf_set_continue_space(pkt_buf_t *buf, int size) {
    debug_assert(size <= PKT_BLK_SIZE, "整合大小大于Block");
    debug_assert(size <= buf->total_size, "整合大小大于包内所有字节数");

    pkt_blk_t *blk = pkt_buf_first_blk(buf);
    if (size <= blk->size) {
        display_check_buf(buf);
        return NET_OK;
    }

    uint8_t *dest = blk->payload;
    for (int i = 0; i < blk->size; i++) {
        *dest++ = blk->data[i];
    }

    int move_size = size - blk->size;
    blk->size += move_size;
    blk->data = blk->payload;

    pkt_blk_t *cur_blk = pkt_blk_next(blk);
    while (move_size) {
        if (cur_blk->size > move_size) {
            plat_memcpy(dest, cur_blk->data, move_size);
            dest += move_size;
            cur_blk->size -= move_size;
            cur_blk->data += move_size;
            break;
        } else {
            plat_memcpy(dest, cur_blk->data, cur_blk->size);
            dest += cur_blk->size;

            pkt_blk_t *next_blk = pkt_blk_next(cur_blk);
            move_size -= cur_blk->size;
            n_list_remove(&buf->list, &cur_blk->node);
            n_locker_lock(&locker);
            m_block_free(&block_list, cur_blk);
            n_locker_unlock(&locker);
            cur_blk = next_blk;
        }
    }

    display_check_buf(buf);
    return NET_OK;
}

void pkt_buf_reset_acc(pkt_buf_t *buf) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (buf) {
        buf->pos = 0;
        buf->cur_blk = pkt_buf_first_blk(buf);
        buf->blk_offset = buf->cur_blk ? buf->cur_blk->data : (void *) 0;
    }
}

static void move_forward(pkt_buf_t *buf, int size) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    buf->pos += size;
    buf->blk_offset += size;

    if (buf->blk_offset >= buf->cur_blk->data + buf->cur_blk->size) {
        buf->cur_blk = pkt_blk_next(buf->cur_blk);
        buf->blk_offset = buf->cur_blk ? buf->cur_blk->data : (void *) 0;
    }
}

net_status_t pkt_buf_write(pkt_buf_t *buf, uint8_t *data, int size) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (!data || !size) {
        return NET_ERROR_PARAM;
    }

    int remain_size = total_blk_remain_relate_to_pos(buf);
    if (remain_size < size) {
        debug(DEBUG_ERROR, "size error: %d < %d", remain_size, size);
        return NET_ERROR_SIZE;
    }

    while (size) {
        int cur_blk_remain_size = pkt_current_blk_remain_size(buf);

        int write_enable_size = cur_blk_remain_size > size ? size : cur_blk_remain_size;
        plat_memcpy(buf->blk_offset, data, write_enable_size);
        size -= write_enable_size;
        data += write_enable_size;

        move_forward(buf, write_enable_size);
    }

    return NET_OK;
}

net_status_t pkt_buf_fill(pkt_buf_t *buf, uint8_t val, int size) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (!size) {
        return NET_ERROR_PARAM;
    }

    int remain_size = total_blk_remain_relate_to_pos(buf);
    if (remain_size < size) {
        debug(DEBUG_ERROR, "size error: %d < %d", remain_size, size);
        return NET_ERROR_SIZE;
    }

    while (size) {
        int cur_blk_remain_size = pkt_current_blk_remain_size(buf);

        int fill_size = cur_blk_remain_size > size ? size : cur_blk_remain_size;
        plat_memset(buf->blk_offset, val, fill_size);
        size -= fill_size;

        move_forward(buf, fill_size);
    }

    return NET_OK;
}

net_status_t pkt_buf_read(pkt_buf_t *buf, uint8_t *data, int size) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (!data || !size) {
        return NET_ERROR_PARAM;
    }

    int remain_size = total_blk_remain_relate_to_pos(buf);
    if (remain_size < size) {
        debug(DEBUG_ERROR, "size error: %d < %d", remain_size, size);
        return NET_ERROR_SIZE;
    }

    while (size) {
        int cur_blk_remain_size = pkt_current_blk_remain_size(buf);

        int read_enable_size = cur_blk_remain_size > size ? size : cur_blk_remain_size;
        plat_memcpy(data, buf->blk_offset, read_enable_size);
        size -= read_enable_size;
        data += read_enable_size;

        move_forward(buf, read_enable_size);
    }

    return NET_OK;
}

net_status_t pkt_buf_seek(pkt_buf_t *buf, int offset) {
    debug_assert(buf->ref != 0, "buf->ref == 0");
    if (buf->pos == offset) {
        return NET_OK;
    }

    debug_assert(buf != (pkt_buf_t *)0, "buf is null");

    if (buf->total_size < offset) {
        debug(DEBUG_ERROR, "size error: %d > %d", offset, buf->total_size);
    }

    int move_bytes = 0;
    if (offset < buf->pos) {
        pkt_buf_reset_acc(buf);
        move_bytes = offset;
    } else {
        move_bytes = offset - buf->pos;
    }

    while (move_bytes) {
        int remain_size = pkt_current_blk_remain_size(buf);
        int cur_move = remain_size > move_bytes ? move_bytes : remain_size;
        move_forward(buf, cur_move);
        move_bytes -= cur_move;
    }
    return NET_OK;
}

net_status_t pkt_buf_copy(pkt_buf_t *dest, pkt_buf_t *src, int size) {
    debug_assert(dest->ref != 0, "dest->ref == 0");
    debug_assert(src->ref != 0, "src->ref == 0");
    if (total_blk_remain_relate_to_pos(dest) < size || total_blk_remain_relate_to_pos(src) < size) {
        return NET_ERROR_SIZE;
    }

    while (size) {
        int dest_remain = pkt_current_blk_remain_size(dest);
        int src_remain = pkt_current_blk_remain_size(src);
        int copy_bytes = dest_remain > src_remain ? src_remain : dest_remain;
        copy_bytes = copy_bytes > size ? size : copy_bytes;

        plat_memcpy(dest->blk_offset, src->blk_offset, copy_bytes);

        move_forward(dest, copy_bytes);
        move_forward(src, copy_bytes);

        size -= copy_bytes;
    }

    return NET_OK;
}

void pkt_buf_inc_ref(pkt_buf_t *buf) {
    n_locker_lock(&locker);
    buf->ref += 1;
    n_locker_unlock(&locker);
}
