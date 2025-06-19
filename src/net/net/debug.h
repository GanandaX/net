//
// Created by Administrator on 2025/5/22.
//

#ifndef NET_DEBUG_H
#define NET_DEBUG_H

#include <stdint.h>
#include "net_config.h"
#include "ipaddr.h"

#define DEBUG_STYLE_INFO "\033[0m"
#define DEBUG_STYLE_WARNING "\033[33m"
#define DEBUG_STYLE_ERROR "\033[31m"
#define DEBUG_STYLE_RESET "\033[0m"

#define DEBUG_LOGGER_LEVEL DEBUG_WARNING
//#define DEBUG_LOGGER_LEVEL DEBUG_INFO

#define DEBUG_INFO      3
#define DEBUG_WARNING   2
#define DEBUG_ERROR     1


void debug_print(unsigned char input_level, char *filePath, const char *func, int line, const char *format, ...);
void dbg_dump_hwaddr(const char *msg, const uint8_t *hwaddr, int len);
void dbg_dump_ip(const char *msg, ipaddr_t *ipaddr);
void dbg_dump_ip_buf(const char *msg, uint8_t *ipaddr);


#define debug(level, format, ...) debug_print(level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define debug_info(level, format, ...) debug_print(level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define debug_warning(level, format, ...) debug_print(level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define debug_error(level, format, ...) debug_print(level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

#define debug_assert(expr, msg) \
    if (!(expr)) {               \
        debug_error(DEBUG_ERROR, "assert failed: "#expr", "msg);\
        while(1);                     \
    }                                 \

#define DEBUG_DISP_ENABLED(module) (module <= DEBUG_WARNING)

#endif //NET_DEBUG_H
