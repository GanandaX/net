//
// Created by Administrator on 2025/6/22.
//
#include "debug.h"
#include "tools.h"

static int is_little_endian(void) {
    // 存储字节顺序，从低地址->高地址
    // 大端：0x12, 0x34;小端：0x34, 0x12
    uint16_t v = 0x1234;
    uint8_t* b = (uint8_t*)&v;

    // 取最开始第1个字节，为0x34,即为小端，否则为大端
    return b[0] == 0x34;
}

/**
 * @brief 工具集初始化
 */
net_status_t tools_init(void) {
    debug(DEBUG_INFO, "init tools.");

    // 实际是小端，但配置项非小端，检查报错
    if (is_little_endian()  != NET_ENDIAN_LITTLE) {
        debug(DEBUG_ERROR, "check endian faild.");
        return NET_ERROR_SYSTEM;
    }

    debug(DEBUG_INFO, "done.");
    return NET_OK;
}


uint16_t checksum16(void *buf, uint16_t len, uint32_t pre_sum, uint8_t complement) {
    uint16_t *curr_buf = (uint16_t *) buf;
    uint32_t checksum = pre_sum;

    while (len > 1) {
        checksum += *curr_buf++;
        len -= 2;
    }

    if (len == 1) {
        checksum += *(uint8_t *) curr_buf;
    }

    uint16_t high;
    while ((high = checksum >> 16) != 0) {
        checksum = (checksum & 0xffff) + high;
    }

    return complement ? ~checksum : checksum;
}
