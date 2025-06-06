//
// Created by Administrator on 2025/5/22.
//
#include "debug.h"
#include "sys.h"
#include "stdarg.h"

void debug_print(const unsigned char input_level, char *filePath, const char *func, int line, const char *format, ...) {

    if (input_level <= DEBUG_LOGGER_LEVEL) {

        static const char *title[] = {
            [DEBUG_INFO] = DEBUG_STYLE_INFO"INFO",
            [DEBUG_WARNING] = DEBUG_STYLE_WARNING"WARNING",
            [DEBUG_ERROR] = DEBUG_STYLE_ERROR"ERROR",
        };

        char *file_name = filePath  + plat_strlen(filePath) - 1;
        while (file_name > filePath) {
            if ((*file_name == '\\') || (*file_name == '/')) {
                break;
            }
            file_name--;
        }

        if ((*file_name == '\\') || (*file_name == '/')) {
            file_name++;
        }

        plat_printf("[%s] %s :: %s [%d] >>  ", title[input_level], file_name, func, line);

        char buf[1024];
        va_list args;

        va_start(args, format);
        plat_vsprintf(buf, format, args);
        plat_printf("%s\n"DEBUG_STYLE_RESET, buf);
        va_end(args);
    }
}
