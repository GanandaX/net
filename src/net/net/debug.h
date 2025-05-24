//
// Created by Administrator on 2025/5/22.
//

#ifndef NET_DEBUG_H
#define NET_DEBUG_H


#define DEBUG_STYLE_INFO "\033[0m"
#define DEBUG_STYLE_WARNING "\033[33m"
#define DEBUG_STYLE_ERROR "\033[31m"
#define DEBUG_STYLE_RESET "\033[0m"

#define DEBUG_INFO      3
#define DEBUG_WARNING   2
#define DEBUG_ERROR     1

void debug_print(unsigned char input_level, unsigned function_level, char *filePath, const char *func, int line, const char *format, ...);

#define debug_info(level, format, ...) debug_print(level, DEBUG_INFO,__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define debug_warning(level, format, ...) debug_print(level, DEBUG_WARNING,__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define debug_error(level, format, ...) debug_print(level, DEBUG_ERROR,__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

#define debug_assert(expr, msg) \
    if (!(expr)) {               \
        debug_error(DEBUG_ERROR, "assert failed: "#expr", "msg);\
        while(1);                     \
    }                                 \



#endif //NET_DEBUG_H
