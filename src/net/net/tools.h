//
// Created by GrandBand on 2025/6/11.
//

#ifndef NET_TOOLS_H
#define NET_TOOLS_H

#include "stdint.h"
#include "net_status.h"

static inline uint16_t swap_u16(uint16_t value) {
    uint16_t result = ((value & 0XFF) << 8) | ((value >> 8) & 0XFF);
    return result;
}

static inline uint32_t swap_u32(uint32_t value) {
    uint32_t result =
            ((value & 0XFF) << 24) | ((value & 0XFF00) << 8) | ((value >> 8) & 0XFF00) | ((value >> 24) & 0XFF);
    return result;
}

#ifdef NET_ENDIAN_LITTLE
#define x_htons(v)  swap_u16(v)
#define x_ntohs(v)  swap_u16(v)
#define x_htohl(v)  swap_u32(v)
#define x_ntohl(v)  swap_u32(v)
#else
#define x_htons(v)  v
#define x_ntohs(v)  v
#define x_htohl(v)  v
#define x_ntohl(v)  v
#endif

net_status_t tools_init(void);

uint16_t checksum16(void *buf, uint16_t len, uint32_t pre_sum, uint8_t complement);


#endif //NET_TOOLS_H
